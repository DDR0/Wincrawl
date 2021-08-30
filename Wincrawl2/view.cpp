#include <iostream>
#include <cassert>
#include <sstream>
#include <limits>

#include "seq.hpp"
#include "color.hpp"

#include "view.hpp"
#include "things.hpp"

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
	
	for (auto offset : std::vector<double>{0.25, 0.75, 0.5, 0}) {
		for (double x = 0; x < viewSize[0]; x += viewSize[0] - 1) {
			for (double y = 0; y < viewSize[1]-1; y++) {
				this->raytrace(rayWalker, viewloc[0], viewloc[1], x, y + offset);
			}
		}
		for (double x = 0; x < viewSize[0]-1; x++) {
			for (double y = 0; y < viewSize[1]; y += viewSize[1] - 1) {
				this->raytrace(rayWalker, viewloc[0], viewloc[1], x + offset, y);
			}
		}
	}
	this->raytrace(rayWalker, viewloc[0], viewloc[1], viewSize[0], viewSize[1]);
	
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

//Borrowed and modified from http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html
//TODO: Fix the off-by-one error in a better way than [8VfkkdY3].
void View::raytrace(RayWalker&, double x0, double y0, double x1, double y1)
{
	double dx = fabs(x1 - x0);
	double dy = fabs(y1 - y0);

	int x = int(floor(x0));
	int y = int(floor(y0));

	int n = 1;
	int x_inc, y_inc;
	double error;
	
	rayWalker.reset(loc, rot, x, y);

	if (dx == 0) {
		x_inc = 0;
		error = std::numeric_limits<double>::infinity();
	}
	else if (x1 > x0) {
		x_inc = 1;
		n += int(floor(x1)) - x;
		error = (floor(x0) + 1 - x0) * dy;
	}
	else {
		x_inc = -1;
		n += x - int(floor(x1));
		error = (x0 - floor(x0)) * dy;
	}

	if (dy == 0) {
		y_inc = 0;
		error -= std::numeric_limits<double>::infinity();
	}
	else if (y1 > y0) {
		y_inc = 1;
		n += int(floor(y1)) - y;
		error -= (floor(y0) + 1 - y0) * dx;
	}
	else {
		y_inc = -1;
		n += y - int(floor(y1));
		error -= (y0 - floor(y0)) * dx;
	}

	for (; n > 0; --n) {
		//std::cerr << "txy: " << x1 << "/" << y1 << ", axy: " << x << "/" << y << ".\n";
		
		if (!rayWalker.moveTo(x, y)) {
			break; //If the rayWalker has moved to an invalid tile, stop tracing.
		};
		
		if (error > 0) {
			y += y_inc;
			error -= dx;
		}
		else {
			x += x_inc;
			error += dy;
		}
		
		//[8VfkkdY3] This algorithm *sometimes* goes one tile too far. I don't know why, but fix it
		//with this hack of a check.
		if ((x0 < x1 && x > x1) || (x0 > x1 && x < x1)) {
			x = x1;
		}
		
		if ((y0 < y1 && y > y1) || (y0 > y1 && y < y1)) {
			y = y1;
		}
	}
}
//*/
	
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