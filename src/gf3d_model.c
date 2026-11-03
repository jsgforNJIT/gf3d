#include "gf3d_model.h"
#include "simple_logger.h"
#include "gf3d_mesh.h"
#include "gfc_text.h"
#include "gf3d_texture.h"
#include "gf3d_swapchain.h"

typedef struct
{
    Model*          model_list;        /**<pre-allocated space for meshes*/
    Texture         texture_default;
    Uint32          max_model;         /**<maximum concurrent sprites supported*/
    Uint32          chain_length;     /**<length of swap chain*/
    VkDevice        device;           /**<logical vulkan device*/
    Pipeline*       pipe;             /**<the pipeline associated with sprite rendering*/
}ModelManager;

static ModelManager modelManagerMain = { 0 }; //for adding and getting rid of mesh in memory

//forward declaration:
void model_init_system(Uint32 modelCount);
void model_free(Model* model);
Model* model_new();
Model* model_load(const char* filename);
void gf3d_model_queue_render(Model* model, GFC_Matrix4 modelMat, GFC_Color colormod);
ModelUBO model_get_ubo(GFC_Matrix4 modelMat, GFC_Color colormod);
Model* model_get_by_filename(const char* filename);
Pipeline* model_get_pipeline();
void model_delete(Model* model);


/*
* @brief initializes the model system and mesh sybsystem
* @param modelCount how many concurrent models will be supported by the system
*/
void model_init_system(Uint32 modelCount) {
    modelManagerMain.max_model = modelCount;
    if (!modelManagerMain.max_model) {
        slog("could not initialize model manager");
        return;
    }

    modelManagerMain.chain_length = gf3d_swapchain_get_chain_length();
    modelManagerMain.device = gf3d_vgraphics_get_default_logical_device();
    modelManagerMain.model_list = gfc_allocate_array(sizeof(Model), modelManagerMain.max_model);
    modelManagerMain.pipe = gf3d_pipeline_create_from_config(
        gf3d_vgraphics_get_default_logical_device(),
        "config/model_pipeline.cfg",
        gf3d_vgraphics_get_view_extent(),
        modelManagerMain.max_model,
        gf3d_mesh_get_bind_description(),
        gf3d_mesh_get_attribute_descriptions(NULL),
        3,
        sizeof(ModelUBO),
        VK_INDEX_TYPE_UINT16
    );
    slog("model_init_system: model manager initialized");
}

void model_free(Model* model) {
    if (!model) return;
    gf3d_mesh_free(model->mesh);
    gf3d_texture_free(model->texture);
    memset(model, 0, sizeof(model));
}

Model* model_new() {
    int emptInd = -1;

    slog("model_new: max num of models: %i", modelManagerMain.max_model);

    for (int i = 0; i < modelManagerMain.max_model; i++) {
        if (modelManagerMain.model_list[i]._refCount != 0) continue;
        //Ask about refcount and inuse
        if (emptInd < 0) emptInd = i;
        if (strlen(modelManagerMain.model_list[i].filename) == 0) {
            //meshManagerMain.mesh_list[i] = tempMesh; //just keep going until there is an empty spot, unless null
            modelManagerMain.model_list[i]._refCount = 1;
            return &modelManagerMain.model_list[i];
        }
    }
    if (emptInd < 0) {
        slog("model_new: no free slots for new models");
        return NULL;
    }

    model_delete(&modelManagerMain.model_list[emptInd]);
    modelManagerMain.model_list[emptInd]._refCount = 1;
    return &modelManagerMain.model_list[emptInd];
}

Model* model_load(const char* filename) {
    Model* tempModel;
    SJson* jsonFile;
    SJson* modelInfo;

    if (!filename) {
        slog("model_load: no file name");
        return NULL;
    }
    slog("model_load: File name exists"); // For debug

    tempModel = model_get_by_filename(filename);
    slog("model_load: model gotten by filename"); // For debug
    if (tempModel) {
        slog("model_load: model %s already exists", filename);
        return tempModel;
    }

    tempModel = model_new();
    if (!tempModel) {
        slog("model_load: model_new() failed");
        return NULL;
    }
    slog("model_load: tempModel gotten from model_new()");

    jsonFile = sj_load(filename);
    if (!jsonFile) {
        slog("model_load: Could not load json file for %s", filename);
        return NULL;
    }
    slog("model_load: sjson file loaded: %s", sj_object_get_string(modelInfo, "model")); // For debug

    modelInfo = sj_object_get_value(jsonFile, "model");
    if (!modelInfo) {
        slog("model_load: Could not load model information from json file for %s", filename);
        return NULL;
    }
    slog("model_load: Model loaded: %s", sj_object_get_string(modelInfo, "model")); // For debug

    tempModel->mesh = gf3d_mesh_load_obj(sj_object_get_string(modelInfo, "obj"));
    if (!tempModel->mesh) {
        slog("model_load: Mesh failed to load for file: %s", filename);
        sj_free(jsonFile);
        return NULL;
    }
    slog("model_load: Mesh loaded: %s", sj_object_get_string(modelInfo, "obj")); // For debug

    tempModel->texture = gf3d_texture_load(sj_object_get_string(modelInfo, "texture"));
    if (!tempModel->texture) {
        slog("model_load: Texture failed to load for file: %s", filename);
        tempModel->texture = gf3d_texture_load("images/default.png");
    }
    slog("model_load: Texture loaded: %s", sj_object_get_string(modelInfo, "texture")); // For debug

    sj_free(jsonFile);

    tempModel->_refCount = 0;
    gfc_line_cpy(tempModel->filename, filename);
    slog("model_load: File name copied: %s", tempModel->filename); // For debug
    if (!tempModel->filename) { // For debug
        slog("model_load: tempModel->filename does not exist");
        return NULL;
    }
    slog("model_load: tempModel->filename exists"); // For debug

    //gfc_list_append(modelManagerMain.model_list, tempModel); //I don't think this is necessary
    slog("model_load: tempModel added to ManagerMain.model_list");
    

    return tempModel;
}

Pipeline* model_get_pipeline() {
    return modelManagerMain.pipe;
}

void gf3d_model_queue_render(Model* model, GFC_Matrix4 modelMat, GFC_Color colormod) {
    ModelUBO modelUBO = model_get_ubo(modelMat, colormod);

    if (!model) {
        //slog("gf3d_model_queue_render: model doesn't exist"); //For debug
        return;
    }
    else {
        //slog("gf3d_model_queue_render: model exists"); //For debug
    }
    if (&modelUBO.color == NULL) {
        slog("gf3d_model_queue_render: modelUBO.color doesn't exist"); //For debug
        return;
    }
    if (&modelUBO.view == NULL) {
        slog("gf3d_model_queue_render: modelUBO.view doesn't exist"); //For debug
        return;
    }
    if (&modelUBO.proj == NULL) {
        slog("gf3d_model_queue_render: modelUBO.proj doesn't exist"); //For debug
        return;
    }
    if (&modelUBO.model == NULL) {
        slog("gf3d_model_queue_render: modelUBO.model doesn't exist"); //For debug
        return;
    }
    //if (model->texture->filename) {
    //    slog("gf3d_model_queue_render: texture being loaded: %s", model->texture->filename); // For debug
    //}

    gf3d_mesh_queue_render(model->mesh, modelManagerMain.pipe, &modelUBO, model->texture);
}

ModelUBO model_get_ubo(GFC_Matrix4 modelMat, GFC_Color colormod) {
    ModelUBO tempModelUBO = { 0 };

    tempModelUBO.color = gfc_color_to_vector4(colormod);
    gfc_matrix4_copy(tempModelUBO.model, modelMat); // World space
    gf3d_vgraphics_get_view(&tempModelUBO.view); // Puts it into camera space
    gf3d_vgraphics_get_projection_matrix(&tempModelUBO.proj); // Makes things smaller based on distance

    return tempModelUBO;
}

void model_delete(Model* model)
{
    if (!model)return;
    gf3d_texture_free(model->texture);
    gf3d_mesh_free(model->mesh);
    memset(model, 0, sizeof(Model));
}


Model* model_get_by_filename(const char* filename) {
    if (!filename) {
        slog("model_get_by_filename: filename doesn't exist");
        return NULL;
    }
    slog("model_get_by_filename: max num of models: %i", modelManagerMain.max_model);

    for (int i = 0; i < modelManagerMain.max_model; i++) {
        //if (!modelManagerMain.model_list[i]._inuse) {
        //    continue;
        //}
        //slog("model_get_by_filename: current index: %i", i); // For debug
        if ((modelManagerMain.model_list[i]._refCount == 0) || (strlen(modelManagerMain.model_list[i].filename) == 0)) {
            //slog("model_get_by_filename: current index not working: %i", i); // For debug
            continue;
        }
        if (gfc_line_cmp(modelManagerMain.model_list[i].filename, filename) == 0) {
            //slog("model_get_by_filename: current index works!: %i", i); // For debug
            return &modelManagerMain.model_list[i];
        }
    }

    slog("model_get_by_filename: couldn't find the model");
    return NULL;
}


/*eol@eof*/