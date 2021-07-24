//Classes related to things, entities, on the map.
#pragma once

#include "color.hpp"

class Tile; //Not defined by places.hpp, as places.hpp requires this first.

class IsVisible {
public:
	const char* glyph{ "￼" };
	Color fgColor{ 0xFF0000 };
	IsVisible() = default;
};

class IsPlayable {
public:
	const int test = 5;
};

template<class... Components>
class Entity: public Components... {
public:
	uint8_t type{ 0 };
	uint8_t zorder{ 0 };
	uint8_t numMixins{ sizeof...(Components) };

	//inline Entity(const Components&... components) : Components(components)... {};
	Entity() = default;
};

class Avatar: public Entity<IsVisible> { //End of the road for this implementation: Can't extend this with further components. Plus it's not dynamic.
public:
	const Tile* loc;

	Avatar() = default;
};