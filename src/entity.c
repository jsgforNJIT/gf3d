#include "simple_logger.h"
#include "entity.h"

typedef struct {
	Entity*			entityList;
	Uint32			maxEnts;
	//Other Stuff Here
}EntityManager;

static EntityManager entity_manager = { 0 };

// Forward declarations
void entity_close();
void entity_system_init(Uint32 maxEnts);
Entity* entity_new();
void entity_free(Entity* ent);
void entity_update(Entity* ent);
void entity_system_update();
void entity_think(Entity* ent);
void entity_system_think();
void entity_draw(Entity* ent);
void entity_system_draw();

void entity_close() {
	for (int i = 0; i < entity_manager.maxEnts; i++) {
		if (entity_manager.entityList[i]._inuse) {
			entity_free(&entity_manager.entityList[i]);
		}
	}
	free(entity_manager.entityList);
	memset(&entity_manager, 0, sizeof(EntityManager));
	slog("entity system closed");
}

void entity_system_init(Uint32 maxEnts) {
	if (!maxEnts) {
		slog("failed to initialize entity system: maxEnts is zero");
		return;
	}
	entity_manager.entityList = gfc_allocate_array(sizeof(Entity), maxEnts);
	if (!entity_manager.entityList) {
		slog("failed to allocate entity list for %i entities", maxEnts);
		return;
	}
	entity_manager.maxEnts = maxEnts;
	slog("entity system initialized");
	atexit(entity_close);
}

Entity* entity_new() {
	for (int i = 0; i < entity_manager.maxEnts; i++) {
		if (entity_manager.entityList[i]._inuse) continue;
		entity_manager.entityList[i]._inuse = 1;
		entity_manager.entityList[i].scale = gfc_vector3d(1, 1, 1);
		entity_manager.entityList[i].colorMod = GFC_COLOR_WHITE;
		return &entity_manager.entityList[i];
	}
	return NULL;
}

void entity_free(Entity* ent) {
	if (!ent) return;
	if (!ent->_inuse) return;
	if (ent->free) ent->free(ent);
	model_free(ent->model);
	memset(ent, 0, sizeof(Entity));
}

/*
* @brief updates this specific entity
*/
void entity_update(Entity* ent) {
	if ((!ent) || (!ent->_inuse)) return;
	if (ent->update) {
		if (ent->update(ent)) return;
	}
	gfc_vector3d_add(ent->position, ent->position, ent->velocity);
	if (gfc_vector3d_magnitude_squared(ent->velocity) > 0.01) {
		gfc_vector3d_scale(ent->velocity, ent->velocity, 0.0);
	}
	else {
		gfc_vector3d_clear(ent->velocity);
	}
}

void entity_system_update() {
	for (int i = 0; i < entity_manager.maxEnts; i++) {
		if (!entity_manager.entityList[i]._inuse) continue;
		entity_update(&entity_manager.entityList[i]);
	}
}

/*
* @brief causes this specific entity to think (like decides what to do)
*/
void entity_think(Entity* ent) {
	if ((!ent) || (!ent->_inuse)) return;
	if (ent->think) {
		if (ent->think(ent)) return;
	}
}

void entity_system_think() {
	for (int i = 0; i < entity_manager.maxEnts; i++) {
		if (!entity_manager.entityList[i]._inuse) continue;
		entity_think(&entity_manager.entityList[i]);
	}
}

/*
* @brief queue render on this specific entity
*/
void entity_draw(Entity* ent) {
	if ((!ent) || (!ent->_inuse)) return;
	if (ent->draw) {
		if (ent->draw(ent)) return;
	}
	//else {
	//	slog("entity_draw: using default draw");
	//}

	// Make sure everything is here
	//if (!ent->mat) {
	//	slog("entity_draw: missing mat");
	//}
	//else {
	//	slog("entity_draw: mat exists");
	//}
	//slog("For pos, rot, and scale:");
	slog("entity_draw: Pos: %f, %f, %f", ent->position.x, ent->position.y, ent->position.z);
	//slog("Rot: %f, %f, %f", ent->rotation.x, ent->rotation.y, ent->rotation.z);
	//slog("Scale: %f, %f, %f", ent->scale.x, ent->scale.y, ent->scale.z);

	gfc_matrix4_from_vectors(ent->mat, ent->position, ent->rotation, ent->scale);
	gf3d_model_queue_render(ent->model,ent->mat,ent->colorMod);
	//slog("Model is rendering");
}


void entity_system_draw() {
	for (int i = 0; i < entity_manager.maxEnts; i++) {
		if (!entity_manager.entityList[i]._inuse) continue;
		entity_draw(&entity_manager.entityList[i]);
	}
}

//int entity_collision_other(Entity *self, Entity *other){
//	GFC_Box a, b;
//	if ((!self)||(!other)) return 0;
//	
//	a = self->bounds;
//	b = other->bounds;
//	
//	a.x = self->position.x;
//	b = b + other->position;
// 
//	//smth in primitives to detect if a and b are colliding, if so then return 1;
//	// Check if self meets b.position < a.position < b.position + w, h, and d for all 3 axises
//	
// 
//	return 0;
// }

/*eol@eof*/