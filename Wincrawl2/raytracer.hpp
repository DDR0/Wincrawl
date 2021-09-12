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
	
	using callback = std::function<void(Tile*, int rayX, int rayY)>;
	struct RaytracerCallbacks { //Callbacks, fired:…
		const callback onEachTile { [](...){} }; //…on each tile the ray passes through.
		const callback onLastTile { [](...){} }; //…on the final tile the ray passes through. May not be the target tile, might have hit something.
		const callback onTargetTile { [](...){} }; //…on the target tile the ray was directed at.
	};
	
public:
	Tile* startingTile{ nullptr };
	int startingDir{ 0 };
	
	const callback onEachTile;
	const callback onLastTile;
	const callback onTargetTile;
	
	Raytracer(const Raytracer::RaytracerCallbacks&);
	
	inline void setOriginTile(Tile* tile, int dir) {
		startingTile = tile; startingDir = dir;
	};
	
	void trace(double sx, double sy, double dx, double dy);
	
	//Debug.
	friend auto operator<<(std::ostream& os, const Raytracer& params) -> std::ostream&;
	friend auto operator<<(std::ostream& os, const Raytracer::RaytracerCallbacks& params) -> std::ostream&;
};