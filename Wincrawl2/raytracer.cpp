#include <iostream>
#include <cassert>
#include <sstream>
#include <limits>

#include "seq.hpp"
#include "color.hpp"

#include "raytracer.hpp"
#include "ecs.hpp"

//So what happens here is that there's two lobes to the brain of this
//raytracer, mainly due to some implementation mismatch around the tiles
//and the ray math. Basically, the raytracer works on a north-facing
//cartesian grid, we have a directed cyclic graph to traverse, and com-
//bining both in one function is prohibitively complex.


//Alternate constructor, so you can specify callbacks when constructing the raytracer.
//Doesn't work, just throws "terminate called after throwing an instance of 'std::bad_function_call'".
Raytracer::Raytracer(const Raytracer::RaytracerCallbacks& callbacks) :
	onEachTile(callbacks.onEachTile),
	onLastTile(callbacks.onLastTile),
	onTargetTile(callbacks.onTargetTile) {};


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
	
	//std::cerr
	//	<< x << "-" << lastX << "=" << currentDeltaX << ", "
	//	<< y << "-" << lastY << "=" << currentDeltaY << "\n";
	
	//This function can only move one step at a time. It's more than complex enough.
	assert(("Out of Range.", abs(currentDeltaX) + abs(currentDeltaY) <= 1));
	
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
	
	onEachTile(loc, x, y);
	return loc && !loc->isOpaque;
}

auto operator<<(std::ostream& os, Raytracer const& params) -> std::ostream& {
	os << "Raytracer at " << params.startingTile << " 🧭" << params.startingDir << "; ";
	
	bool cb{ false };
	if(params.onEachTile) {
		os << "onEachTile=" << params.onEachTile.target_type().name();
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

//TODO: Use the algorithm from http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html (The implementation there doesn't seem to work, overshoots target.)
void Raytracer::trace(double sx, double sy, double dx, double dy) {
	reset(sx, sy);
	
	decltype(loc) oldloc;
	const int steps = std::max(abs(sx-dx), abs(sy-dy));
	int lastY{ sy }; //We move in a zig-zag pattern, so x and y do not change simultaneously. (We don't have diagonal links in our tiling system.)
	
	
	//Change step to 0 to trace including the starting tile.
	//Change the conditional to < to avoid covering the destination tile.
	int step{ 1 };
	for (;step <= steps;) {
		const int x{ round(sx + (dx-sx) * step/steps) };
		const int y{ round(sy + (dy-sy) * step/steps) };
		
		//Advance the raywalker to the tile. Stop couldn't move there.
		oldloc = loc;
		if (!moveTo(x, lastY)) {
			onLastTile(loc, lastX, lastY);
			break;
		}
		oldloc = loc;
		if (!moveTo(x, y)) {
			onLastTile(loc, lastX, lastY);
			break;
		}
		
		lastY = y;
		
		step++;
	}
	
	if(step == steps) {
		onTargetTile(loc, lastX, lastY);
	}
	
}

auto operator<<(std::ostream& os, Raytracer::RaytracerCallbacks const& params) -> std::ostream& {
	bool cb{ false };
	if(params.onEachTile) {
		os << "onEachTile=" << params.onEachTile.target_type().name();
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