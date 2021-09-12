#include <iostream>
#include <cassert>
#include <sstream>
#include <limits>

#include "view.hpp"
#include "color.hpp"
#include "seq.hpp"


View::View(uint8_t width, uint8_t height, Tile* pointOfView)
	: loc(pointOfView)
{
	viewSize[0] = width;
	viewSize[1] = height;

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
}

void View::render(std::ostream& target) {
	assert(loc); //If no location is defined, fail.
	raytracer.setOriginTile(loc, rot);

	int viewloc[2] = { viewSize[0] / 2, viewSize[1] / 2 };

	//First, all our tiles are hidden.
	for (int x = 0; x < viewSize[0]; x++) {
		for (int y = 0; y < viewSize[1]; y++) {
			grid[x][y] = &hiddenTile;
		}
	}
	
	//TODO: Rework this so it traces the lines around true (integer) lines first, then the final true lines.
	for (auto offset : std::vector<double>{0.25, 0.75, 0.5, 0}) {
		for (double x = 0; x < viewSize[0]; x += viewSize[0] - 1) {
			for (double y = 0; y < viewSize[1]-1; y++) {
				raytracer.trace(viewloc[0], viewloc[1], x, y + offset);
			}
		}
		for (double x = 0; x < viewSize[0]-1; x++) {
			for (double y = 0; y < viewSize[1]; y += viewSize[1] - 1) {
				raytracer.trace(viewloc[0], viewloc[1], x + offset, y);
			}
		}
	}
	
	//Trace the final diagonal line to the 1-2 corner, which doesn't get covered otherwise.
	raytracer.trace(viewloc[0], viewloc[1], viewSize[0], viewSize[1]);
	
	//We don't ever trace the center tile, just those around it.
	grid[viewloc[0]][viewloc[1]] = loc;

	std::ostringstream buffer;

	for (int y = 0; y < viewSize[1]; y++) {
		for (int x = 0; x < viewSize[0]; x++) {
			//Print entity on tile.
			for (auto entity : grid[x][y]->occupants) {
				auto paint = entity->dispatch(Event::GetRendered{});
				if (!paint.glyph) continue;
				
				buffer
					<< paint.fgColor.fg() << grid[x][y]->bgColor.bg() //Just ignore background color for now, need a "none" or "alpha" variant.
					<< reinterpret_cast<const char*>(paint.glyph)
					<< seq::reset;
				
				goto nextTile;
			}
			
			//If no entities, print tile itself.
			buffer
				<< grid[x][y]->fgColor.fg() << grid[x][y]->bgColor.bg()
				<< reinterpret_cast<const char*>(grid[x][y]->glyph)
				<< seq::reset;
			
			nextTile: continue;
		}
		buffer << "\n";
	}

	target << buffer.str();
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
	
	static bool hasMoved = false;
	if (!hasMoved) {
		hasMoved = true;
		std::cout << seq::clear; //TODO: Move this to somewhere more appropriate? We'll probably want to composit squares together for different viewpoints and text.
	}
	std::cout << "[0;0H"; //Move the cursor to 0,0. See previous comment.

	render(std::cout);
}

void View::turn(int delta) {
	rot = (delta + rot + 4) % 4;
	
	std::cout << seq::clear;
	render(std::cout);
}