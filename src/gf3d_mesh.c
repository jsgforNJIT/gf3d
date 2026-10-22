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

#define MESH_ATTRIBUTE_COUNT 3

//forward declaration:
typedef struct ObjData_S ObjData;
void gf3d_mesh_primitive_free(MeshPrimitive* prim);
Mesh* gf3d_mesh_get_by_filename(const char* filename);
Mesh* gf3d_mesh_load(const char* filename, int bound_x, int bound_y, int bound_z, int bound_w, int bound_h, int bound_d);


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

/**
 * @brief initializes the mesh system / configures internal data about mesh based rendering
 * @param mesh_max the maximum allowed simultaneous meshes supported at once.  Must be > 0
 * @note keep in mind that many models will be comprised of multiple sub meshes.  So this number may need to be very large
 */
void gf3d_mesh_init(Uint32 mesh_max) {
    Uint32 count;

    meshManagerMain.max_mesh = mesh_max;
    if (!meshManagerMain.max_mesh) {
        slog("could not initialize mesh manager");
        return;
    }
    meshManagerMain.mesh_list = (Mesh *)gfc_allocate_array(sizeof(Mesh), meshManagerMain.max_mesh);

    gf3d_mesh_get_attribute_descriptions(&count);
    meshManagerMain.pipe = gf3d_pipeline_create_from_config(
        gf3d_vgraphics_get_default_logical_device(),
        "config/overlay_pipeline.cfg",
        gf3d_vgraphics_get_view_extent(),
        mesh_max,
        gf3d_mesh_get_bind_description(),
        gf3d_mesh_get_attribute_descriptions(NULL),
        count,
        sizeof(MeshUBO),
        VK_INDEX_TYPE_UINT16
    );
}

/**
 * @brief get a new empty model
 * @return NULL on error, or an empty model
 */
Mesh* gf3d_mesh_new() {
    //Mesh tempMesh = { 0 }; I don't think I need to make an empty mesh yet
    for (int i = 0; i < meshManagerMain.max_mesh; i++) {
        if (strlen(meshManagerMain.mesh_list[i].filename) == 0) {
            //meshManagerMain.mesh_list[i] = tempMesh; //just keep going until there is an empty spot, unless null
            return &meshManagerMain.mesh_list[i];
        }
    }
    slog("gf3d_mesh_new: no free slots for new meshes");
    return NULL;
    //You can do things where the last object is deleted to do so.
}


void gf3d_mesh_delete(Mesh* mesh)
{
    if (!mesh)return;
    for (int i = mesh->primitives->count; i > 0; i--) {
        if (!gfc_list_nth((mesh->primitives), i)) {
            continue;
        }
        gf3d_mesh_primitive_free(gfc_list_nth((mesh->primitives), i));
    }

    gfc_list_delete(mesh->primitives);
    memset(mesh, 0, sizeof(mesh));
    //It's model that deals with texture.
}

void gf3d_mesh_primitive_free(MeshPrimitive* prim) { //Does both the prim and its buffers
    
    if (prim->vertexBuffer != VK_NULL_HANDLE){          //Clear prim buffers
        vkDestroyBuffer(meshManagerMain.device, prim->vertexBuffer, NULL);
    }
    if (prim->vertexBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(meshManagerMain.device, prim->vertexBufferMemory, NULL);
    }
    if (prim->faceBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(meshManagerMain.device, prim->faceBuffer, NULL);
    }
    if (prim->faceBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(meshManagerMain.device, prim->faceBufferMemory, NULL);
    }
    
    if (prim->objData) {                                //Clear prim objData
        gf3d_obj_free(prim->objData);
    }
    free(prim);                                         //Clear prim itself
}

/**
 * @brief load mesh data from an obj filename.
 * @note: currently only supporting obj files
 * @note this free's the intermediate data loaded from the obj file, no longer needed for most applications
 * @param filename the name of the file to load
 * @return NULL on error or Mesh data
 */
Mesh* gf3d_mesh_load_obj(const char* filename) { // Compare to gf2d_sprite_load_image -> sprite_load
    return gf3d_mesh_load(filename, 0, 0, 0, 0, 0, 0);
}

Mesh* gf3d_mesh_load(const char* filename, int bound_x, int bound_y, int bound_z, int bound_w, int bound_h, int bound_d) {
    Mesh* tempMesh;
    MeshPrimitive* tempMeshPrim;
    tempMesh = gf3d_mesh_get_by_filename(filename);
    ObjData* objFromFile = gf3d_obj_load_from_file(filename);


    if (tempMesh) {
        tempMesh->_inuse++;
        return tempMesh;
    }

    tempMesh = gf3d_mesh_new();
    if (!tempMesh) {
        return NULL;
    }
    tempMeshPrim = gfc_allocate_array(sizeof(MeshPrimitive), 1);
    tempMeshPrim->objData = objFromFile;
    gfc_list_append(tempMesh->primitives, tempMeshPrim);
    //Skip over texture
    //Should do something about the primitives, return here later
    tempMesh->_refCount = 0;

    if (bound_x <= 0) bound_x = objFromFile->bounds.x;
    if (bound_y <= 0) bound_y = objFromFile->bounds.y;
    if (bound_z <= 0) bound_z = objFromFile->bounds.z;
    if (bound_w <= 0) bound_w = objFromFile->bounds.w;
    if (bound_h <= 0) bound_h = objFromFile->bounds.h;
    if (bound_d <= 0) bound_d = objFromFile->bounds.d;
    tempMesh->bounds.x = bound_x;
    tempMesh->bounds.y = bound_y;
    tempMesh->bounds.z = bound_z;
    tempMesh->bounds.w = bound_w;
    tempMesh->bounds.h = bound_h;
    tempMesh->bounds.d = bound_d;

    gfc_line_cpy(tempMesh->filename, filename);

    


    //gf3d_obj_free(objFromFile);
    return tempMesh;
}

Mesh* gf3d_mesh_get_by_filename(const char* filename) {
    if (!filename)return NULL;
    for (int i = 0; i < meshManagerMain.max_mesh; i++) {
        if (!meshManagerMain.mesh_list[i]._inuse) {
            continue;
        }
        if (gfc_line_cmp(meshManagerMain.mesh_list[i].filename, filename) == 0) {
            return &meshManagerMain.mesh_list[i];
        }
    }
    return NULL;
}

//All removed (copy, move, primitive_new) also removed in the newest version by EngineerOfLies

/**
 * @brief get the input attribute descriptions for mesh based rendering
 * @param count (optional, output) the number of attributes
 * @return a pointer to a vertex input attribute description array
 */
VkVertexInputAttributeDescription* gf3d_mesh_get_attribute_descriptions(Uint32* count) {
    meshManagerMain.attributeDescriptions[0].binding = 0;
    meshManagerMain.attributeDescriptions[0].location = 0;
    meshManagerMain.attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    meshManagerMain.attributeDescriptions[0].offset = offsetof(Vertex, vertex);

    meshManagerMain.attributeDescriptions[1].binding = 0;
    meshManagerMain.attributeDescriptions[1].location = 1;
    meshManagerMain.attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    meshManagerMain.attributeDescriptions[1].offset = offsetof(Vertex, texel);
    if (count)*count = MESH_ATTRIBUTE_COUNT;
    return meshManagerMain.attributeDescriptions;
}

/**
 * @brief get the binding description for mesh based rendering
 * @return vertex input binding descriptions compatible with mesh data
 */
VkVertexInputBindingDescription* gf3d_mesh_get_bind_description() {
    meshManagerMain.bindingDescription.binding = 0;
    meshManagerMain.bindingDescription.stride = sizeof(Vertex);
    meshManagerMain.bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return &meshManagerMain.bindingDescription;
}

/**
 * @brief free a mesh that has been loaded from memory
 */
void gf3d_mesh_free(Mesh* mesh) {
    if (!mesh) return;
    mesh->_inuse--;
    if (mesh->_inuse <= 0) gf3d_mesh_delete(mesh);
    //make sure to free the primitives and other datatypes, too
}

//All removed (reset_pipes, submit_pipe_commands, get_model_command_buffer) also removed in the newest version by EngineerOfLies

/**
 * @brief queue up a render for the current draw frame
 * @param mesh the mesh to render
 * @param pipe the pipeline to use
 * @param uboData the data to use to draw the mesh
 * @param texture texture data to use
 */
void gf3d_mesh_queue_render(Mesh* mesh, Pipeline* pipe, void* uboData, Texture* texture) {
    MeshPrimitive* tempPrim;
    //Do for each primitive of the mesh
    for (int i = 0; i < mesh->primitives->count; i++) {
        tempPrim = gfc_list_get_nth(mesh->primitives, i);
        gf3d_pipeline_queue_render(pipe, tempPrim->vertexBuffer, tempPrim->vertexCount, tempPrim->faceBuffer, &uboData, texture);
    }
    
}


//All removed (mesh_render) also removed in the newest version by EngineerOfLies

/**
 * @brief render a mesh through a given pipeline
 */
void gf3d_mesh_render_generic(Mesh* mesh, Pipeline* pipe, VkDescriptorSet* descriptorSet) {
    MeshPrimitive* tempPrim;
    //Do for each primitive of the mesh
    for (int i = 0; i < mesh->primitives->count; i++) {
        tempPrim = gfc_list_get_nth(mesh->primitives, i);
        gf3d_pipeline_call_render(pipe, descriptorSet, tempPrim->vertexBuffer, tempPrim->vertexCount, tempPrim->faceBuffer);
    }
}

/**
 * @brief create a mesh's internal buffers based on vertices
 * @param primitive the mesh primitive to populate
 * @note the primitive must have the objData set and it must have be organizes in buffer order
 */
void gf3d_mesh_create_vertex_buffer_from_vertices(MeshPrimitive* primitive) {/*TODO*/
    void* data = NULL;
    VkDevice device = meshManagerMain.device;
    size_t bufferSize = sizeof(Vertex) * primitive->objData->vertex_count;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Vertex* vertices = primitive->objData->vertices;

    gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &primitive->vertexBuffer, &primitive->vertexBufferMemory);

    gf3d_buffer_copy(stagingBuffer, primitive->vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
}
//Learn what this means


//All removed (get_ubo) also removed in the newest version by EngineerOfLies

Pipeline* gf3d_mesh_get_pipeline() {
    return meshManagerMain.pipe;
}

/*eol@eof*/