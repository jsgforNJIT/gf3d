#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "simple_json.h"
#include "gfc_types.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_text.h"
#include "gf3d_model.h"

typedef struct {
	Uint8			_inuse;
	Model*			model;
	GFC_Vector3D	position;
	GFC_Vector3D	rotation;
	GFC_Vector3D	scale;
	GFC_Matrix4		mat;
	GFC_Vector3D	velocity;
	GFC_Color		colorMod;
}Entity;


/*
* @brief initialize the entity manager and pre-allocate a bunch
* @param maxEnts how many entities can exist at any single moment
*/
void entity_init(Uint32 maxEnts);

/*
* @brief get  ablank entity from the system
* @return NULL on error or out of memory. a valid pointer otherwise
*/
Entity* entity_new();

/*
* @brief call to clean up an entity after it is no longer needed
*/
void entity_free(Entity* ent);

/*
* @brief run updates on all active entities
*/
void entity_system_update();

/*
* @brief run think functions on all active entities
*/
void entity_system_think();

/*
* @brief queue renders on all active entities
*/
void entity_system_draw();

#endif