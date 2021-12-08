#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>

#include "color.hpp"
#include "seq.hpp"
#include "view.hpp"

auto red = Color(0xFF0000FF);

View::View(uint8_t width, uint8_t height, Tile* pointOfView)
	: loc(pointOfView)
{
	viewSize[0] = width;
	viewSize[1] = height;
	
	std::cout << "init loc pre: " << grid.size() << "\n";

	grid.reserve(width);
	for (int x = 0; x < viewSize[0]; x++) {
		grid.emplace_back()
			.assign(height, nullptr);
	}
	
	//A few tiles are needed as placeholders by the rendering code.
	hiddenTile.roomId = 1;
	hiddenTile.glyph = "░";
	emptyTile.roomId = 2;
	emptyTile.glyph = "▓";
	
	//raytracer.onEachTile = [&](auto loc, auto x, auto y){
	//	grid[x][y] = loc ? loc : &emptyTile;
	//};
	
	std::cout << "init loc post: " << grid.size() << "\n";
}

void View::render(std::ostream& target) {
	
	//Trace the final diagonal line to the 1-2 corner, which doesn't get covered otherwise.
	raytracer.trace(0, 0, 0, 0);
}


void View::render(std::unique_ptr<TextCellSubGrid> target) {
	std::cout << "render loc pre: " << grid.size() << "\n";
	
	grid.clear();
	grid.reserve(50);
	for (int x = 0; x < 50; x++) {
		grid.emplace_back().assign(50, nullptr);
	}
	
	std::cout << "render loc post 1: " << grid.size() << "\n";

	raytracer.trace(0, 0, 0, 0);
	
	std::cout << "render loc post 2: " << grid.size() << "\n";
}


void View::render() {
	std::cout << "render loc pre: " << grid.size() << "\n";
	
	grid.clear();
	grid.reserve(50);
	for (int x = 0; x < 50; x++) {
		grid.emplace_back().assign(50, nullptr);
	}
	
	std::cout << "render loc post 1: " << grid.size() << "\n";

	raytracer.trace(0, 0, 0, 0);
	
	std::cout << "render loc post 2: " << grid.size() << "\n";
}

	
void View::moveCamera(int direction) {
	auto link {loc->getNextTile((direction + rot + 4) % 4) };
	if (!link->tile()) return;

	//To get the new view rotation, consider the following example
	//of travelling between two tiles in direction 1.
	//
	//  view.rot=1    view.rot=0
	//  ┌─────┐       ┌─────┐
	//  │  1  │ dir 1 │  0  │
	//  │0   2│ ────→ │3   1│
	//  │  3  │       │  2  │
	//  └─────┘       └─────┘
	//   tile1         tile2
	//
	//To calculate the new view rotation, we get our rotational
	//delta by subtracting the opposite of the destination arrival
	//direction (which we entered from) from the direction we left
	//from. In this case, we leave from 2 and arrive at 3, so the
	//opposite direction is 1. This is pointing, relatively
	//speaking, in the same direction we left. We will keep it
	//aligned by adding the delta between the two directions to
	//our rotation. Delta is 1-2=-1, so rotation is 1+-1=0.
	//
	//Thus, the following formula breaks down like so:
	//Inner-most brackets: Tile direction left in.
	//Up one level: Rotatinoal delta calculation.
	//Rest of formula: Add rotational delta to current rotation.
	
	//Hack: Drag the first entity found on a tile to the tile we're moving to. This should always be our player, plus we have no other entities for now. We should just render the entity's tile.
	Entity* player { loc->occupants.back() };
	loc->occupants.pop_back();
	
	rot = (rot + (
		Tile::oppositeEdge[link->dir()] - (direction + rot)
	) + 4) % 4;
	loc = link->tile();
	
	loc->occupants.push_back(player);
}

void View::move(int direction) {
	moveCamera(direction);
}

void View::turn(int delta) {
	rot = (delta + rot + 4) % 4;
	
	std::cout << seq::clear;
	render(std::cout);
}