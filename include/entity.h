#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "simple_json.h"
#include "gfc_types.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_text.h"
#include "gf3d_model.h"
#include "gfc_list.h"

typedef struct EntityS{ //EntityS so that Entity can refer to itself
	Uint8			_inuse;
	Model*			model;
	GFC_Vector3D	position;
	GFC_Vector3D	rotation;
	GFC_Vector3D	scale;
	GFC_Matrix4		mat; // model matrix
	GFC_Vector3D	velocity;
	GFC_Color		colorMod;
	GFC_Box			bounds; // hitboxes and stuff like that, found in meshPrimitives from the file // Don't forget to set bounds in mesh
	void*			data; // Has data in it dependant on class 
	char*			category; // Is it a player, interactable, projectile, etc.
	int				selfInd; // Index within entity manager
	GFC_List*		collWith; // Currently unused, doing hard-coding for now // list of strings to collide with

	int (*draw)(struct EntityS* self); // These stuff are so that anything using entity can call these instead of this entity version.
	int	(*free)(struct EntityS* self);
	int	(*think)(struct EntityS* self);
	int	(*update)(struct EntityS* self);
}Entity;

typedef struct {
	Entity* entityList;
	Uint32			maxEnts;
}EntityManager;

/*
* @brief initialize the entity manager and pre-allocate a bunch
* @param maxEnts how many entities can exist at any single moment
*/
void entity_system_init(Uint32 maxEnts);

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

/*
* @brief return whether or not these two entities are colliding
* @param self one of the entities that may be colliding
* @param other the other entities that may be colliding
*/
int entity_collision_other(Entity* self, Entity* other);

/*
* @brief allows the returning of the entity manager
*/
EntityManager* entity_system_get();

#endif