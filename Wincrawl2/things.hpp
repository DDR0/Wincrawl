//Classes related to things, entities, on the map.
#pragma once

#include "color.hpp"

class Tile; //Not defined by places.hpp, as places.hpp requires this first.

template<class... Components>
class Entity: public Components... {
public:
	const char* glyph{ "￼" };
	Color fgColor{ 0xFF0000 };
	uint8_t type{ 0 };
	uint8_t zorder{ 0 };
	uint8_t numMixins{ sizeof...(Components) };

	inline Entity(const Components&... components) : Components(components)... {};
};

class Avatar: public Entity<> {
	const Tile* loc;
};