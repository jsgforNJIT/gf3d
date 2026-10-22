//#include "simple_logger.h"
//#include "gfc_input.h"
//#include "gf3d_vgraphics.h"
//#include "player.h"
//
//int player_think()
//
//typedef struct {
//	float			cameraFollowDistance;
//	GFC_Vector2D	cameraRotation;
//	//other player specific data, such as xp, level, strength, etc.
//}PlayerEntityData;
//
//int player_think(Entity* self);
//
//Entity* player_spawn(GFC_Vector3D position) {
//	Entity* self;
//	self = entity_new();
//	if (!self) return NULL;
//	self->model = model_load("models/dino/dino.model");
//	self->position = position;
//	self->velocity = player_think;
//	self->free = player_free;
//	self->data = gfc_allocate_array(sizeof(PlayerEntityData), 1);
//	return self;
//}
//
//void player_free(Entity* self) {
//	PlayerEntityData* data;
//	if ((!self) || (!self->data)) return;
//	data = (PlayerEntityData*)self->data;
//	free(data);
//	self->data = NULL;
//}
//
//int player_think(Entity* self) {
//	GFC_Matrix4* view;
//	GFC_Vector3D move = { 0 };
//	GFC_Vector3D offset = { 0, -50, 0 };	
//	if ((!self) || (!self->data)) return 0;
//	PlayerEntityData* data = self->data;
//
//	view = gf3d_vgraphics_get_view_matrix();
//	gfc_vector3d_add(offset, offset, self->position);
//	gfc_matrix_view(*view, offset, self->position, gfc_vector3d(0, 0, 1));
//	
//
//
//	//movement
//	if (gfc_input_command_down("item_up")) move.y = 1;
//	if (gfc_input_command_down("item_down")) move.y = -1;
//	if (gfc_input_command_down("item_left")) move.x = 1;
//	if (gfc_input_command_down("item_right")) move.x = -1;
//
//	if (move.x || move.y) {
//		gfc_vector3d_normalize(&move);
//		self->velocity = move;
//	}
//	gfc_vector3d_scale(offset, offset, data->cameraFollowDistance);
//	view = gf3d_vgraphics_get_view_matrix();
//	gfc_vector3d_rotate_about_z(&offset, data->cameraRotation.x);
//	gfc_vector3d_add(offset, offset, self->position);
//	gfc_matrix_view(*view, offset, self->position, gfc_vector3d(0, 0, 1));
//
//	return 0;
//}
//
///**/