#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_vgraphics.h"
#include "player.h"
#include "gfc_matrix.h"
#include "gfc_text.h"
#include "entity.h"

typedef struct {
	float			cameraFollowDistance;
	GFC_Vector2D	cameraRotation;
	float			speed;
	//other player specific data, such as xp, level, strength, etc.
}PlayerEntityData;

// Forward declarations
int player_think(Entity* self);
void player_free(Entity* self);
void player_update(Entity* self);

Entity* player_spawn(GFC_Vector3D position) {
	Entity* self;
	PlayerEntityData* tempData;

	self = entity_new();
	if (!self) return NULL;
	self->model = model_load("models/dino.model");
	if (!self->model) {
		slog("player_spawn: model didn't load");
	}
	gfc_matrix4_identity(self->mat);
	self->colorMod = GFC_COLOR_WHITE;

	self->position = position;
	self->velocity = gfc_vector3d(0, 0, 0);
	self->free = player_free;
	self->think = player_think;
	self->update = player_update;
	self->category = "player";
	self->collWith = gfc_list_new(); // Currently unused

	self->bounds.x = -5;
	self->bounds.y = -5;
	self->bounds.z = -5;
	self->bounds.w = 10;
	self->bounds.h = 10;
	self->bounds.d = 10;

	tempData = gfc_allocate_array(sizeof(PlayerEntityData), 1);

	// Data
	tempData->speed = 1; // Change to be not hard-coded
	tempData->cameraFollowDistance = 1;

	self->data = tempData;
	slog("player_spawn: player is spawned");
	return self;
}

void player_free(Entity* self) {
	PlayerEntityData* data;
	if ((!self) || (!self->data)) return;
	data = (PlayerEntityData*)self->data;
	free(data);
	self->data = NULL;
}

int player_think(Entity* self) {
	GFC_Matrix4* view;
	GFC_Vector3D move = { 0 };
	GFC_Vector3D offset = { 0, -100, 50};	
	if ((!self) || (!self->data)) return 0;
	PlayerEntityData* data = self->data;
	float speed = data->speed;

	view = gf3d_vgraphics_get_view_matrix();

	//movement
	if (gfc_input_command_down("item_up")) {
		move.y = 1;
		//slog("player_think: moving up");
	}
	if (gfc_input_command_down("item_down")) {
		move.y = -1;
		//slog("player_think: moving down");
	}
	if (gfc_input_command_down("item_right")) {
		move.x = 1;
		//slog("player_think: moving left");
	}
	if (gfc_input_command_down("item_left")) {
		move.x = -1;
		//slog("player_think: moving right");
	}
	//move.y = 1; // For debug
	if (move.x || move.y) {
		gfc_vector3d_normalize(&move);
		gfc_vector3d_set_magnitude(&move, speed);
		self->velocity = move;
	}
	gfc_vector3d_scale(offset, offset, data->cameraFollowDistance);
	view = gf3d_vgraphics_get_view_matrix();
	//gfc_vector3d_rotate_about_z(&offset, data->cameraRotation.x);
	gfc_vector3d_add(offset, offset, self->position);
	gfc_matrix4_view(*view, offset, self->position, gfc_vector3d(0, 0, 1));

	//slog("player_think: offset: %f, %f, %f \n position: %f, %f, %f", //offset by x seems to be twice the position
	//	offset.x, offset.y, offset.z, self->position.x, self->position.y, self->position.z);

	return 0;
}

void player_update(Entity* self) {
	int collWithWall = false; // Checks if already interacted with wall
	EntityManager* entSys = entity_system_get();
	gfc_vector3d_add(self->position, self->position, self->velocity);
	//slog("player_think: current player: %s", self->category); // For debug

	for (int i = 0; i < entSys->maxEnts; i++) {
		if (!entSys->entityList[i]._inuse) continue;
		if (self->selfInd == i) continue;
		//slog("player_update: currently on %s", entSys->entityList[i].category);
		if (entity_collision_other(self, &entSys->entityList[i])) {
			slog("player_update: collision detected for %s", entSys->entityList[i].category);
			if ((gfc_stricmp(entSys->entityList[i].category, "wall") == 0) && (!collWithWall)) {
				slog("player_update: player inside wall");
				gfc_vector3d_sub(self->position, self->position, self->velocity);
				collWithWall = true;
			}
			if ((entSys->entityList[i].category == "goo") && (!collWithWall)) {
				//There's this thing just to double check
			}
		}
	}

	if (gfc_vector3d_magnitude_squared(self->velocity) > 0.01) {
		gfc_vector3d_scale(self->velocity, self->velocity, 0.0);
	}
	else {
		gfc_vector3d_clear(self->velocity);
	}
}

/**/