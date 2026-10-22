#include "gf3d_model.h"
#include "simple_logger.h"

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
    gfc_allocate_array(sizeof(Model), modelManagerMain.max_model);
    modelManagerMain.pipe = gf3d_mesh_get_pipeline();
}

void model_free(Model* model) {
    if (!model) return;
    gf3d_mesh_free(model->mesh);
    gf3d_texture_free(model->texture);
    memset(model, 0, sizeof(model));
}

Model* model_load(const char* filename) {
    Model* tempModel;

    if (!filename) {
        slog("No file name");
        return NULL;
    }
    gfc_line_cpy(tempModel->filename, filename);

    tempModel->mesh = gf3d_mesh_load_obj(filename);
    if (!tempModel->mesh) {
        slog("Mesh failed to load for file: %s", filename);
        return NULL;
    }

    tempModel->texture = gf3d_texture_load(filename);
    if (!tempModel->texture) {
        slog("Texture failed to load for file: %s", filename);
        return NULL;
    }

    tempModel->_refCount = 0;
    
    gfc_list_append(modelManagerMain.model_list, tempModel);

    return tempModel;
}

Pipeline* model_get_pipeline() {
    return modelManagerMain.pipe;
}

void gf3d_model_queue_render(Model* model) {
    ModelUBO modelUBO = { 0 };

    gf3d_mesh_queue_render(model->mesh, modelManagerMain.pipe, &modelUBO, model->texture);
}

ModelUBO model_get_ubo(GFC_Matrix4 modelMat, GFC_Color colormod) {
    ModelUBO tempModelUBO = { 0 };

    tempModelUBO.color = gfc_color_to_vector4(colormod);
    gfc_matrix4_copy(tempModelUBO.model, modelMat);
    gfc_matrix4_copy(tempModelUBO.view, gf3d_vgraphics_get_view_matrix());
    // Missing proj

    return tempModelUBO;
}

Model* model_get_by_filename(const char* filename) {
    if (!filename)return NULL;
    for (int i = 0; i < modelManagerMain.max_model; i++) {
        //if (!modelManagerMain.model_list[i]._inuse) {
        //    continue;
        //}
        if (gfc_line_cmp(modelManagerMain.model_list[i].filename, filename) == 0) {
            return &modelManagerMain.model_list[i];
        }
    }
    return NULL;
}


/*eol@eof*/