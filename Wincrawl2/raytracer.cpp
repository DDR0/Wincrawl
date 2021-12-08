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
}

//Move tile stepper.
bool Raytracer::moveTo(int x, int y) { //Must be within the bounds of field.
	onEachTile(loc, x, y);
	return true;
}

//TODO: Use the algorithm from http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html (The implementation there doesn't seem to work, overshoots target.)
void Raytracer::trace(double sx, double sy, double dx, double dy) {
	moveTo(sx, sy);
}

