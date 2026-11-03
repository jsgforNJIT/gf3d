#include <stdalign.h>
#include <vulkan/vulkan.h>
#include "gf3d_mesh.h"

#include "gfc_vector.h"
#include "gfc_list.h"
#include "gfc_text.h"
#include "gfc_matrix.h"
#include "gfc_primitives.h"
#include "simple_logger.h"

#include "gf3d_pipeline.h"
#include "gf3d_obj_load.h"
#include "gf3d_vgraphics.h"
#include "gf3d_buffers.h"

#define MESH_ATTRIBUTE_COUNT 3

//forward declaration:
typedef struct ObjData_S ObjData;
void gf3d_mesh_init(Uint32 mesh_max);
void gf3d_mesh_manager_close();
Mesh* gf3d_mesh_new();
void gf3d_mesh_delete(Mesh* mesh);
void gf3d_mesh_primitive_free(MeshPrimitive* prim);
Mesh* gf3d_mesh_load_obj(const char* filename);
Mesh* gf3d_mesh_get_by_filename(const char* filename);
VkVertexInputAttributeDescription* gf3d_mesh_get_attribute_descriptions(Uint32* count);
VkVertexInputBindingDescription* gf3d_mesh_get_bind_description();
void gf3d_mesh_queue_render(Mesh* mesh, Pipeline* pipe, void* uboData, Texture* texture);
void gf3d_mesh_render_generic(Mesh* mesh, Pipeline* pipe, VkDescriptorSet* descriptorSet);
void gf3d_mesh_create_primitive_buffer(MeshPrimitive* primitive);


typedef struct
{
    Mesh*           mesh_list;        /**<pre-allocated space for meshes*/
    Uint32          max_mesh;         /**<maximum concurrent sprites supported*/
    Uint32          chain_length;     /**<length of swap chain*/
    VkDevice        device;           /**<logical vulkan device*/
    Pipeline*       pipe;             /**<the pipeline associated with sprite rendering*/
    VkVertexInputAttributeDescription   attributeDescriptions[MESH_ATTRIBUTE_COUNT];
    VkVertexInputBindingDescription     bindingDescription;
}MeshManager;


static MeshManager meshManagerMain = { 0 }; //for adding and getting rid of mesh in memory

void gf3d_mesh_init(Uint32 mesh_max) {
    Uint32 count;

    if (mesh_max == 0) {
        slog("gf3d_mesh_init: mesh_max must be greater than 0");
    }
    meshManagerMain.max_mesh = mesh_max;
    if (!meshManagerMain.max_mesh) {
        slog("gf3d_mesh_init: could not set mesh_max");
        return;
    }
    meshManagerMain.mesh_list = (Mesh *)gfc_allocate_array(sizeof(Mesh), meshManagerMain.max_mesh);
    if (!meshManagerMain.mesh_list) {
        slog("gf3d_mesh_init: could not set mesh_list");
        return;
    }

    meshManagerMain.device = gf3d_vgraphics_get_default_logical_device();
    if (!meshManagerMain.device) {
        slog("gf3d_mesh_init: could not set device");
        return;
    }

    atexit(gf3d_mesh_manager_close);
    gf3d_mesh_get_attribute_descriptions(&count);
    meshManagerMain.pipe = gf3d_pipeline_create_from_config(
        gf3d_vgraphics_get_default_logical_device(),
        "config/model_pipeline.cfg",
        gf3d_vgraphics_get_view_extent(),
        mesh_max,
        gf3d_mesh_get_bind_description(),
        gf3d_mesh_get_attribute_descriptions(NULL),
        count,
        sizeof(MeshUBO),
        VK_INDEX_TYPE_UINT16
    );
}

void gf3d_mesh_manager_close() {
    for (int i = 0; i < meshManagerMain.max_mesh; i++) {
        gf3d_mesh_delete(&meshManagerMain.mesh_list[i]);
    }
    free(meshManagerMain.mesh_list);
    memset(&meshManagerMain, 0, sizeof(meshManagerMain));
}

Mesh* gf3d_mesh_new() {
    for (int i = 0; i < meshManagerMain.max_mesh; i++) {
        if (meshManagerMain.mesh_list[i]._refCount != 0) continue;
        //Ask about refcount and inuse
        if (strlen(meshManagerMain.mesh_list[i].filename) == 0) {
            //meshManagerMain.mesh_list[i] = tempMesh; //just keep going until there is an empty spot, unless null
            meshManagerMain.mesh_list[i].primitives = gfc_list_new();
            if (meshManagerMain.mesh_list[i].primitives == NULL) {
                slog("gf3d_mesh_new: couldn't add primitive memory to index %i of mesh_list", i);
                return NULL;
            }
            slog("gf3d_mesh_new: found available memory index %i of mesh_list", i);
            meshManagerMain.mesh_list[i]._refCount = 1;
            return &meshManagerMain.mesh_list[i];
        }
    }

    slog("gf3d_mesh_new: no free slots for new meshes");
    return NULL;
    //You can do things where the last object is deleted to do so.
}

void gf3d_mesh_delete(Mesh* mesh) {
    if (!mesh)return;
    if (mesh->_refCount > 0) mesh->_refCount--;
    if (mesh->_refCount > 0) return;


    if (mesh->primitives) {
        for (int i = mesh->primitives->count; i > 0; i--) {
            if (!gfc_list_nth((mesh->primitives), i)) {
                continue;
            }
            gf3d_mesh_primitive_free(gfc_list_nth((mesh->primitives), i));
        }
        gfc_list_delete(mesh->primitives);
    }
    
    memset(mesh, 0, sizeof(mesh));
    //It's model that deals with texture.
}

void gf3d_mesh_primitive_free(MeshPrimitive* prim) { //Does both the prim and its buffers
    
    if (!prim) return;
    if (prim->vertexBuffer != VK_NULL_HANDLE){          //Clear prim buffers
        vkDestroyBuffer(meshManagerMain.device, prim->vertexBuffer, NULL);
        prim->vertexBuffer = VK_NULL_HANDLE;
    }
    if (prim->vertexBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(meshManagerMain.device, prim->vertexBufferMemory, NULL);
        prim->vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (prim->faceBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(meshManagerMain.device, prim->faceBuffer, NULL);
        prim->faceBuffer = VK_NULL_HANDLE;
    }
    if (prim->faceBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(meshManagerMain.device, prim->faceBufferMemory, NULL);
        prim->faceBufferMemory = VK_NULL_HANDLE;
    }
    
    if (prim->objData) {                                //Clear prim objData
        gf3d_obj_free(prim->objData);
    }
    free(prim);                                         //Clear prim itself
}

Mesh* gf3d_mesh_load_obj(const char* filename) { // Compare to gf2d_sprite_load_image -> sprite_load
    Mesh* tempMesh;
    MeshPrimitive* tempMeshPrim;
    ObjData* objFromFile;
    
    slog("gf3d_mesh_load_obj: yup, I can do stuff");
    if (!filename) {
        slog("No filename");
        return NULL;
    }
    slog("gf3d_mesh_load_obj: filename exists"); // For debug

    tempMesh = gf3d_mesh_get_by_filename(filename);
    slog("gf3d_mesh_load_obj: mesh gotten by filename"); // For debug
    if (tempMesh) {
        slog("gf3d_mesh_load_obj: mesh %s already exists", filename);
        tempMesh->_refCount++;
        return tempMesh;
    }

    tempMesh = gf3d_mesh_new();
    if (!tempMesh) {
        slog("gf3d_mesh_load_obj: gf3d_mesh_new() failed");
        return NULL;
    }
    slog("gf3d_mesh_load_obj: tempModel gotten from gf3d_mesh_new()");

    objFromFile = gf3d_obj_load_from_file(filename);
    if (!objFromFile) {
        slog("gf3d_mesh_load_obj: couldn't get object file for %s", filename);
        return NULL;
    }
    slog("gf3d_mesh_load_obj: obj loaded from file"); // For debug

    tempMeshPrim = gfc_allocate_array(sizeof(MeshPrimitive), 1);
    if (!tempMeshPrim) {
        slog("gf3d_mesh_load_obj: could not allocate memory for mesh primitive");
        gf3d_mesh_delete(tempMesh);
        return NULL;
    }
    slog("gf3d_mesh_load_obj: mesh primitive memory allocated"); // For debug

    tempMeshPrim->objData = objFromFile;
    if (!tempMeshPrim->objData) {
        slog("gf3d_mesh_load_obj: could not get object data from %s", filename);
        gf3d_mesh_primitive_free(tempMeshPrim);
        gf3d_mesh_delete(tempMesh);
        return NULL;
    }
    slog("gf3d_mesh_load_obj: tempMeshPrim->objData gotten from objFromFile"); // For debug

    gf3d_mesh_create_primitive_buffer(tempMeshPrim);
    if ((!tempMeshPrim->faceBuffer) || (!tempMeshPrim->faceBufferMemory) || (!tempMeshPrim->vertexBuffer) || (!tempMeshPrim->vertexBufferMemory)) {
        slog("gf3d_mesh_load_obj: missing faceBuffer/faceBufferMemory/vertexBuffer/vertexBufferMemory");
        gf3d_mesh_primitive_free(tempMeshPrim);
        gf3d_mesh_delete(tempMesh);
        return NULL;
    }
    slog("gf3d_mesh_load_obj: objFromFile face count: %i", tempMeshPrim->faceCount);

    if (tempMeshPrim->faceCount != objFromFile->face_count) {
        slog("gf3d_mesh_load_obj: could not make primitive buffers");
        gf3d_mesh_primitive_free(tempMeshPrim);
        gf3d_mesh_delete(tempMesh);
        return NULL;
    }


    gfc_list_append(tempMesh->primitives, tempMeshPrim);
    tempMesh->_refCount = 0;
    gfc_line_cpy(tempMesh->filename, filename);
    return tempMesh;
}

Mesh* gf3d_mesh_get_by_filename(const char* filename) {
    if (!filename)return NULL;
    for (int i = 0; i < meshManagerMain.max_mesh; i++) {
        if (meshManagerMain.mesh_list[i]._refCount == 0) {
            continue;
        }
        if (gfc_line_cmp(meshManagerMain.mesh_list[i].filename, filename) == 0) {
            return &meshManagerMain.mesh_list[i];
        }
    }
    return NULL;
}

//All removed (copy, move, primitive_new) also removed in the newest version by EngineerOfLies

VkVertexInputAttributeDescription* gf3d_mesh_get_attribute_descriptions(Uint32* count) {
    meshManagerMain.attributeDescriptions[0].binding = 0;
    meshManagerMain.attributeDescriptions[0].location = 0;
    meshManagerMain.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    meshManagerMain.attributeDescriptions[0].offset = offsetof(Vertex, vertex);

    meshManagerMain.attributeDescriptions[1].binding = 0;
    meshManagerMain.attributeDescriptions[1].location = 1;
    meshManagerMain.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    meshManagerMain.attributeDescriptions[1].offset = offsetof(Vertex, normal);

    meshManagerMain.attributeDescriptions[2].binding = 0;
    meshManagerMain.attributeDescriptions[2].location = 2;
    meshManagerMain.attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    meshManagerMain.attributeDescriptions[2].offset = offsetof(Vertex, texel);
    if (count)*count = MESH_ATTRIBUTE_COUNT;
    return meshManagerMain.attributeDescriptions;
}

VkVertexInputBindingDescription* gf3d_mesh_get_bind_description() {
    meshManagerMain.bindingDescription.binding = 0;
    meshManagerMain.bindingDescription.stride = sizeof(Vertex);
    meshManagerMain.bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return &meshManagerMain.bindingDescription;
}

void gf3d_mesh_free(Mesh* mesh) {
    if (!mesh) return;
    if (mesh->_refCount > 0) mesh->_refCount--;
    if (mesh->_refCount <= 0) gf3d_mesh_delete(mesh);
    //make sure to free the primitives and other datatypes, too
}

//All removed (reset_pipes, submit_pipe_commands, get_model_command_buffer) also removed in the newest version by EngineerOfLies

void gf3d_mesh_queue_render(Mesh* mesh, Pipeline* pipe, void* uboData, Texture* texture) {
    MeshPrimitive* tempPrim;

    if (!mesh) {
        slog("gf3d_mesh_queue_render: missing mesh");
        return;
    }
    if (!pipe) {
        slog("gf3d_mesh_queue_render: missing pipe");
        return;
    }
    if (!uboData) {
        slog("gf3d_mesh_queue_render: missing uboData");
        return;
    }
    if (!texture) {
        slog("gf3d_mesh_queue_render: missing texture");
        return;
    }

    slog("gf3d_mesh_queue_render: texture name: %s", texture->filename);
    //slog("gf3d_mesh_queue_render: total number of primitives: %i", gfc_list_count(mesh->primitives)); // For debug

    //Do for each primitive of the mesh
    for (int i = 0; i < gfc_list_count(mesh->primitives); i++) {
        tempPrim = gfc_list_get_nth(mesh->primitives, i);
        if (!tempPrim) {
            slog("gf3d_mesh_queue_render: failed to get index %i from %s", i, mesh->filename);
            return;
        }
        if (!(tempPrim->vertexBuffer && tempPrim->vertexCount && tempPrim->faceBuffer)) {
            slog("gf3d_mesh_queue_render: failed to get an attribute from %s", mesh->filename);
            return;
        }
        //slog("gf3d_mesh_queue_render: current primitive num: %i", i); // For debug
        gf3d_pipeline_queue_render(pipe, tempPrim->vertexBuffer, tempPrim->vertexCount, tempPrim->faceBuffer, uboData, texture);
    }
    
}

//All removed (mesh_render) also removed in the newest version by EngineerOfLies

void gf3d_mesh_render_generic(Mesh* mesh, Pipeline* pipe, VkDescriptorSet* descriptorSet) {
    MeshPrimitive* tempPrim;
    //Do for each primitive of the mesh
    for (int i = 0; i < mesh->primitives->count; i++) {
        tempPrim = gfc_list_get_nth(mesh->primitives, i);
        gf3d_pipeline_call_render(pipe, descriptorSet, tempPrim->vertexBuffer, tempPrim->vertexCount, tempPrim->faceBuffer);
    }
}

void gf3d_mesh_create_primitive_buffer(MeshPrimitive* primitive) {
    void* data = NULL;
    VkDevice device = meshManagerMain.device;
    size_t bufferSize;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    if ((!primitive) || (!primitive->objData)) {
        slog("gf3d_mesh_create_primitive_buffer: primitive doesn't exist");
        return;
    }
    slog("gf3d_mesh_create_primitive_buffer: primitive exists"); // For debug

    // Vertex
    bufferSize = sizeof(Vertex) * primitive->objData->face_vert_count;
    gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, primitive->objData->faceVertices, bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &primitive->vertexBuffer, &primitive->vertexBufferMemory);
    gf3d_buffer_copy(stagingBuffer, primitive->vertexBuffer, bufferSize);
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
    primitive->vertexCount = primitive->objData->face_vert_count;

    slog("gf3d_mesh_create_primitive_buffer: primitive vertex count: %i", primitive->vertexCount); // For debug

    // Face
    bufferSize = sizeof(Face) * primitive->objData->face_count;
    gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, primitive->objData->outFace, bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &primitive->faceBuffer, &primitive->faceBufferMemory);
    gf3d_buffer_copy(stagingBuffer, primitive->faceBuffer, bufferSize);
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
    primitive->faceCount = primitive->objData->face_count;

    slog("gf3d_mesh_create_primitive_buffer: primitive face count: %i", primitive->faceCount); // For debug
}
//Learn what this means


//All removed (get_ubo) also removed in the newest version by EngineerOfLies

Pipeline* gf3d_mesh_get_pipeline() {
    return meshManagerMain.pipe;
}

/*eol@eof*/