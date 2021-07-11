//Classes related to things, entities, on the map.
#pragma once

#include "color.hpp"
#include "places.hpp"

class Tile; //Not defined by places.hpp, as places.hpp requires this first.

class Entity {
public:
	const char* glyph{ "￼" };
	Color fgColor{ 0xFF0000 };
	uint8_t type { 0 };
	uint8_t zorder { 0 };
};

class Avatar: public Entity {
	const Tile* loc;
};