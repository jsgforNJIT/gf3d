#ifndef __GF3D_MODEL_H__
#define __GF3D_MODEL_H__

#include "gf3d_mesh.h"
#include "gfc_matrix.h"
#include "gf3d_vgraphics.h"

typedef struct {
	GFC_Matrix4		model;
	GFC_Matrix4		view;
	GFC_Matrix4		proj;
	GFC_Vector4D	color;
}ModelUBO;

typedef struct {
	int				_refCount;
	Mesh		   *mesh;
	Texture		   *texture;
	GFC_TextLine	filename;
}Model;

/*
* @brief initializes the model system and mesh sybsystem
* @param modelCount how many concurrent models will be supported by the system
*/
void model_init_system(Uint32 modelCount);

void model_free(Model* model);

Model* model_load(const char* filename);

Pipeline* model_get_pipeline();

void gf3d_model_queue_render(Model* model);

ModelUBO model_get_ubo(GFC_Matrix4 modelMat, GFC_Color colormod);

#endif