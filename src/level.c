//#include "simple_logger.h"
//#include "level.h"
//
//GFC_Vector3D level_line_test(Level* level, GFC_Edge3D edge) {
//	Mesh *mesh;
//	MeshPrimitive* prim;
//	GFC_Triangle3D t;
//	int i, c, j;
//	GFC_Vector3D out =  - 1;
//	if ((!level) || (!level->terrain) || (!level->terrain->mesh)) return 0;
//	//c = gfc_list_count(level->terrain->)
//	Uint8 gfc_triangle_edge_test(GFC_Edge3D e, GFC_Triangle3D t, GFC_Vector3D * contact);
//}
//
//Level* level_new() {
//	Level* level;
//	level = gfc_allocate_array(sizeof(Level), 1);
//	if (!level) {
//		slog("failed to allocate level data");
//		return NULL;
//	}
//	gfc_matrix4_identity(level->terrainMatrix);
//	return level;
//}
//
//Level* level_load(const char* filename) {
//	SJson* json;
//	SJson* data;
//	const char* str;
//	Level* level;
//	json = sj_load(filename);
//
//	if (!json) {
//		slog("failed to open level file %s", filename);
//		return NULL;
//	}
//
//	data = sj_object_get_value(json, "level");
//	if (!data) {
//		sj_free(json);
//		slog("file %s does not contain level data", filename);
//		return NULL;
//	}
//	
//	level = level_new();
//	if (!level) return NULL;
//
//}
//
//void level_free(Level* level);
//
//void level_draw(Level* level);
//
///*eol@eof*/