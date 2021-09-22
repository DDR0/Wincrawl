#pragma once

#include <functional>

#include "places.hpp"
#include "ecs.hpp"
#include "raytracer.hpp"

class View {
	//A view is a regular grid of tiles, as seen from a specific tile.
	//Because our tiles are non-euclidean, you may see a tile multiple times.

	uint8_t viewSize[2];
	std::vector<std::vector<Tile*>> grid;
	inline static Tile hiddenTile{};
	inline static Tile emptyTile{};
	
	Raytracer raytracer{{
		.onEachTile = [&](auto loc, auto x, auto y){
			grid[x][y] = loc ? loc : &emptyTile;
		}
	}};

public:
	Tile* loc;
	int rot{ 0 };

	View(uint8_t width, uint8_t height, Tile* pointOfView);

	void render(std::ostream& target);
	
	void moveCamera(int direction);
	
	void move(int direction);
	void turn(int delta);
};