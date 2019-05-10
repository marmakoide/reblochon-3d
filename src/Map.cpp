#include "Map.h"

using namespace reb;



Map::Cell::Cell() :
	m_height(0),
	m_wall_texture_id(0),
	m_floor_texture_id(0) { }



Map::Map() :
	m_cell_array(1, 1) { }



Map::Map(int w, int h)
	: m_cell_array(w, h) { }
