#include <iostream>
#include <cassert>
#include <sstream>
#include <limits>

#include "seq.hpp"
#include "color.hpp"

#include "raytracer.hpp"
#include "things.hpp"

//So what happens here is that there's two lobes to the brain of this
//raytracer, mainly due to some implementation mismatch around the tiles
//and the ray math. Basically, the raytracer works on a north-facing
//cartesian grid, we have a directed cyclic graph to traverse, and com-
//bining both in one function is prohibitively complex.

//Reset the tile stepper.
void Raytracer::reset(int x, int y) {
	loc = startingTile;
	dir = startingDir;

	lastX = x;
	lastY = y;
	lastDirectionIndex = 0;
}

//Move tile stepper.
bool Raytracer::moveTo(int x, int y) { //Must be within the bounds of field.
	const int currentDeltaX = x - lastX;
	const int currentDeltaY = y - lastY;
	
	//This function can only move one step at a time. It's more than complex enough.
	assert(("Out of Range.", abs(currentDeltaX) + abs(currentDeltaY) > 1));
	
	//std::cout << "moved to " << x << "×" << y << " (" << currentDeltaX << "×" << currentDeltaY << ")\n";
	int directionIndex;
	if (currentDeltaY == +1) { directionIndex = 0; } else
	if (currentDeltaX == +1) { directionIndex = 1; } else
	if (currentDeltaY == -1) { directionIndex = 2; } else
	if (currentDeltaX == -1) { directionIndex = 3; } else {
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
		//(*field)[x][y] = loc;
		return !loc->isOpaque; //Continue tracing only if whatever we're looking at isn't solid.
	}
	else {
		//std::cout << "setting " << x << "×" << y << " to #" << emptyTile.getIDStr() << " (" << reinterpret_cast<const char*>(emptyTile.glyph) << ")\n";
		//(*field)[x][y] = &emptyTile;
		return false;
	}
}

auto operator<<(std::ostream& os, Raytracer const& params) -> std::ostream& {
	os << "Raytracer at " << params.startingTile << " 🧭" << params.startingDir << "; ";
	
	bool cb{ false };
	if(params.onAllTiles) {
		os << "onAllTiles=" << params.onAllTiles.target_type().name();
		cb |= true;
	}
	if(params.onLastTile) {
		if(cb) os << ", ";
		os << "onLastTile=" << params.onLastTile.target_type().name();
		cb |= true;
	}
	if(params.onTargetTile) {
		if(cb) os << ", ";
		os << "onTargetTile=" << params.onTargetTile.target_type().name();
		cb |= true;
	}
	if(!cb) {
		os << "no callbacks.";
	}
	
	os << "\n";
	return os;
}

void Raytracer::raytrace(double sx, double sy, double dx, double dy) {
	reset(sx, sy);
	
	const int steps = std::max(abs(sx-dx), abs(sy-dy));
	int lastY{ sy }; //We move in a zig-zag pattern, so x and y do not change simultaneously. (We don't have diagonal links in our tiling system.)
	
	//Change step to 0 to trace including the starting tile.
	//Change the conditional to < to avoid covering the destination tile.
	for (int step{ 1 }; step <= steps; step++) {
		const int x{ round(sx + (dx-sx) * step/steps) };
		const int y{ round(sy + (dy-sy) * step/steps) };
		
		//Advance the raywalker to the tile. Stop couldn't move there.
		if (!moveTo(x, lastY)) break;
		if (!moveTo(x, y)) break;
		
		lastY = y;
	}
}