#ifndef __OBSTACLE_H__
#define __OBSTACLE_H__

#include "simple_json.h"
#include "gfc_types.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_text.h"
#include "entity.h"

Entity* wall_spawn(GFC_Vector3D position);

#endif