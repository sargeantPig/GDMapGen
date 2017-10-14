
#include "register_types.h"
#include "genmap.h"
#ifndef _3D_DISABLED
#include "object_type_db.h"
#endif

void register_mapGen_types() {
	ObjectTypeDB::register_type<Noisey>();
}

void unregister_mapGen_types() {
}
