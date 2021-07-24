#include <iostream>
#include <cassert>

#include "seq.hpp"
#include "color.hpp"

#include "view.hpp"

View::RayWalker::RayWalker(std::vector<std::vector<Tile*>>* field_) : field(field_) {}

void View::RayWalker::reset(Tile* startingTile, int rot, int x, int y) {
	loc = startingTile;
	dir = rot;

	lastX = x;
	lastY = y;
	lastDirectionIndex = 0;
}

bool View::RayWalker::moveTo(int x, int y) { //Must be within the bounds of field.
	const int currentDeltaX = x - lastX;
	const int currentDeltaY = y - lastY;

	//std::cout << "moved to " << x << "×" << y << " (" << currentDeltaX << "×" << currentDeltaY << ")\n";
	int directionIndex;
	if (currentDeltaY == +1) directionIndex = 0; else
	if (currentDeltaX == +1) directionIndex = 1; else
	if (currentDeltaY == -1) directionIndex = 2; else
	if (currentDeltaX == -1) directionIndex = 3; else {
		return true; //No motion, stay where we are.
	}

	if (not(lastX || lastY)) {
		//Starting off, so relative movement to our tile.
		auto movement = loc->getNextTile(directionIndex);
		loc = movement->tile();
		lastDirectionIndex = dir = movement->dir();
		//std::cout << "moved! " << directionIndex << "\n";
	}
	else {
		//Enter the room in the relative direction from us.
		//std::cout << "moved: " << directionIndex << " (from " << lastDirectionIndex << " is " << (directionIndex-lastDirectionIndex) << ")\n";
		auto movement = loc->getNextTile(dir, directionIndex - lastDirectionIndex);
		loc = movement->tile();
		dir = movement->dir();
		lastDirectionIndex = directionIndex;
	}

	lastX = x;
	lastY = y;

	if (loc) {
		//std::cout << "setting " << x << "×" << y << " to #" << loc->getIDStr() << " (" << reinterpret_cast<const char*>(loc->glyph) << ")\n";
		(*field)[x][y] = loc;
		return !loc->isOpaque; //Continue tracing only if whatever we're looking at isn't solid.
	}
	else {
		//std::cout << "setting " << x << "×" << y << " to #" << emptyTile.getIDStr() << " (" << reinterpret_cast<const char*>(emptyTile.glyph) << ")\n";
		(*field)[x][y] = &emptyTile;
		return false;
	}
}
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
}

void View::render(std::ostream& target) {
	assert(loc); //If no location is defined, fail.

	int viewloc[2] = { viewSize[0] / 2, viewSize[1] / 2 };

	//First, all our tiles are hidden.
	for (int x = 0; x < viewSize[0]; x++) {
		for (int y = 0; y < viewSize[1]; y++) {
			grid[x][y] = &hiddenTile;
		}
	}

	for (int x = 0; x < viewSize[0]; x += viewSize[0] - 1) {
		for (int y = 0; y < viewSize[1]; y++) {
			this->raytrace(rayWalker, viewloc[0], viewloc[1], x, y);
		}
	}
	for (int x = 0; x < viewSize[0]; x++) {
		for (int y = 0; y < viewSize[1]; y += viewSize[1] - 1) {
			this->raytrace(rayWalker, viewloc[0], viewloc[1], x, y);
		}
	}
	
	//We don't ever trace the center tile, just those around it.
	grid[viewloc[0]][viewloc[1]] = loc;

	for (int y = 0; y < viewSize[1]; y++) {
		for (int x = 0; x < viewSize[0]; x++) {
			//Print entity on tile.
			for (auto entity : grid[x][y]->occupants) {
				target
					<< entity->fgColor.fg() << grid[x][y]->bgColor.bg()
					<< reinterpret_cast<const char*>(entity->glyph)
					<< seq::reset;
				goto nextTile;
			}
			
			//If no entities, print tile itself.
			target
				<< grid[x][y]->fgColor.fg() << grid[x][y]->bgColor.bg()
				<< reinterpret_cast<const char*>(grid[x][y]->glyph)
				<< seq::reset;
			
			nextTile: continue;
		}
		target << "\n";
	}
}

//Borrowed and modified from http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html
//TODO: Shoot more rays, presumably using the floating-point version.
void View::raytrace(RayWalker&, int x0, int y0, int x1, int y1)
{
	int dx{ abs(x1 - x0) };
	int dy{ abs(y1 - y0) };
	int x{ x0 };
	int y{ y0 };
	int x_inc{ (x1 > x0) ? 1 : -1 };
	int y_inc{ (y1 > y0) ? 1 : -1 };
	int error = dx - dy;
	dx *= 2;
	dy *= 2;

	rayWalker.reset(loc, rot, x, y);

	for (int n = (dx + dy) / 2; n > 0; --n) {
		if (error > 0) {
			x += x_inc;
			error -= dy;
		}
		else {
			y += y_inc;
			error += dx;
		}

		if (!rayWalker.moveTo(x, y)) {
			break; //If the rayWalker has moved to an invalid location, stop tracing.
		};
	}
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
	Entity<IsVisible>* player { loc->occupants.back() };
	loc->occupants.pop_back();
	
	rot = (rot + (
		Tile::oppositeEdge[link->dir()] - (direction + rot)
	) + 4) % 4;
	loc = link->tile();
	
	loc->occupants.push_back(player);
}
	
void View::move(int direction) {
	moveCamera(direction);
	
	std::cout << seq::clear; //TODO: Move this to somewhere more appropriate? We'll probably want to composit squares together for different viewpoints and text.
	render(std::cout);
}

void View::turn(int delta) {
	rot = (delta + rot + 4) % 4;
	
	std::cout << seq::clear;
	render(std::cout);
}