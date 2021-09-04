#pragma once

#include <functional>
#include <vector>

#include "places.hpp"
#include "things.hpp"

class Raytracer {
	//Since our geometry has no external location or orientation, we must "walk" it to
	//find out what we've got. Our RayWalker translates absolute movement of the raytrace
	//function into relative movement through the world.
	//It notes down what it's found in the field.
	
	Tile* loc{ nullptr }; //location
	int dir{ 0 }; //direction
	
	int lastX{ 0 };
	int lastY{ 0 };
	int lastDirectionIndex{ 0 };
	
	typedef std::vector<std::vector<Tile*>>* Field;
	Field field{};
	
	void reset(int x, int y);
	
	bool moveTo(int x, int y);
	
public:
	Tile* startingTile;
	int startingDir{ 0 };
	
	//Callbacks, fired…:
	const std::function<void(Tile*)> onAllTiles { [](auto _){} }; //…on each tile the ray passes through.
	const std::function<void(Tile*)> onLastTile { [](auto _){} }; //…on the final tile the ray passes through. May not be the target tile, might have hit something.
	const std::function<void(Tile*)> onTargetTile { [](auto _){} }; //…on the target tile the ray was directed at.
	
	inline Raytracer(Tile* startingTile_, int startingDir_):
		startingTile(startingTile_), startingDir(startingDir_) {};
	
	void raytrace(double sx, double sy, double dx, double dy);
	
	
	//Debug.
	friend auto operator<<(std::ostream& os, Raytracer const& params) -> std::ostream&;
};