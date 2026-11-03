#include "simple_logger.h"
#include "item.h"

Entity* item_spawn(const char* itemName, GFC_Vector3D position) {
	const char* str;
	SJson* itemDef;
	Entity* self;

	if (!itemName) return NULL;
	itemDef = gfc_config_def_get_by_name("items", ItemName);
	self = entity_new();
	str = sj_object_get_string(itemDef, "model");
	if (self) {
		self->model = model_load(str);
	}
	self->position = position;
	self->scale = gfc_vector3d(100, 100, 100);

	return self;
}

/*eol@eof*/