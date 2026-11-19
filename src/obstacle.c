#include "obstacle.h"

#include "simple_json.h"
#include "simple_logger.h"
#include "gfc_types.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_text.h"
#include "entity.h"

// Forward declaration
int wall_think(Entity* self);

Entity* wall_spawn(GFC_Vector3D position, GFC_Vector3D dimensions) {
	Entity* self;
	//PlayerEntityData* tempData;

	self = entity_new();
	if (!self) return NULL;
	self->model = model_load("models/dino.model");
	if (!self->model) {
		slog("obstacle_spawn: model didn't load");
	}
	self->scale = gfc_vector3d(dimensions.x/10, dimensions.y / 10, dimensions.z / 10);
	gfc_matrix4_identity(self->mat);
	self->colorMod = GFC_COLOR_WHITE;

	self->position = position;
	self->velocity = gfc_vector3d(0, 0, 0);
	self->category = "wall";
	self->collWith = gfc_list_new(); // Currently unused
	slog("wall_spawn: wall spawned");

	self->think = wall_think;

	self->bounds.x = -(dimensions.x/2);
	self->bounds.y = -(dimensions.y / 2);
	self->bounds.z = -(dimensions.z / 2);
	self->bounds.w = dimensions.x;
	self->bounds.h = dimensions.y;
	self->bounds.d = dimensions.z;

	//tempData = gfc_allocate_array(sizeof(PlayerEntityData), 1);

	//// Data
	//tempData->speed = 1; // Change to be not hard-coded
	//tempData->cameraFollowDistance = 1;

	self->data = gfc_allocate_array(0, 1);
	slog("wall_spawn: wall is spawned");
	return self;
}


int wall_think(Entity* self) {
	return true;
}
/**/