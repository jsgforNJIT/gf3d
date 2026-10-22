//#include "simple_logger.h"
//#include "entity.h"
//
//typedef struct {
//	Entity*			entityList;
//	Uint32			maxEnts;
//	//Other Stuff Here
//}EntityManager;
//
//static EntityManager entity_manager = { 0 };
//
//void entity_close() {
//	for (int i = 0; i < entity_manager.maxEnts; i++) {
//		if (entity_manager.entityList[i]._inuse) {
//			entity_free(&entity_manager.entityList[i]);
//		}
//	}
//	free(entity_manager.entityList);
//	memset(&entity_manager, 0, sizeof(EntityManager));
//	slog("entity system closed");
//}
//
///*
//* @brief initialize the entity manager and pre-allocate a bunch
//* @param maxEnts how many entities can exist at any single moment
//*/
//void entity_init(Uint32 maxEnts) {
//	if (!maxEnts) {
//		slog("failed to initialize entity system: maxEnts is zero");
//		return;
//	}
//	entity_manager.entityList = gfc_allocate_array(sizeof(Entity), maxEnts);
//	if (!entity_manager.entityList) {
//		slog("failed to allocate entity list for %i entities", maxEnts);
//		return;
//	}
//	entity_manager.maxEnts = maxEnts;
//	slog("entity system initialized");
//	atexit(entity_close);
//}
//
///*
//* @brief get  ablank entity from the system
//* @return NULL on error or out of memory. a valid pointer otherwise
//*/
//Entity* entity_new() {
//	for (int i = 0; i < entity_manager.maxEnts; i++) {
//		if (entity_manager.entityList[i]._inuse) continue;
//		entity_manager.entityList[i]._inuse = 1;
//		//Boilerplate code goes here
//		entity_manager.entityList[i].scale = gfc_vector3d(1, 1, 1);
//		entity_manager.entityList[i].colorMod = GFC_COLOR_WHITE;
//		return &entity_manager.entityList[i];
//	}
//	return NULL;
//}
//
///*
//* @brief call to clean up an entity after it is no longer needed
//*/
//void entity_free(Entity* ent) {
//	if (!ent) return;
//	if (!ent->_inuse) return;
//	if (ent->free) ent->free(ent);
//	model_free(ent->model);
//	memset(ent, 0, sizeof(Entity));
//}
//
//void entity_update(Entity* ent) {
//	if ((!ent) || (!ent->_inuse)) return;
//	if (ent->update) {
//		if (ent->update(ent)) return;
//	}
//	//gfc_vector3d_add(self->position, self->position, self->velocity);
//	//if (gfc_vector3d_magnitude_squared(self->velocity) > 0.01) {
//	//	gfc_vector3d_scale(self->velocity, self->velocity, 0.0);
//	//}
//	//else {
//	//	gfc_vector3d_clear(self->velocity);
//	//}
//}
//
///*
//* @brief run updates on all active entities
//*/
//void entity_system_update();
//
///*
//* @brief run think functions on all active entities
//*/
//void entity_system_think();
//
//
///*
//* 
//*/
//void entity_draw(Entity* ent) {
//	if ((!ent) || (!ent->_inuse)) return;
//	if (ent->draw) {
//		if (ent->draw(ent)) return;
//	}
//	gfc_matrix4_from_vectors(ent->mat, ent->position, ent->rotation, ent->scale);
//	model_queue_render(ent->model.ent->mat.ent->color);
//}
//
///*
//* @brief queue renders on all active entities
//*/
//void entity_system_draw() {
//	for (int i = 0; i < entity_manager.maxEnts; i++) {
//		if (!entity_manager.entityList[i]._inuse) continue;
//		entity_draw(&entity_manager.entityList[i]);
//	}
//}
//
///*eol@eof*/