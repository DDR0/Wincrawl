#pragma once

#include <functional>

#include "places.hpp"
#include "things.hpp"

class View {
	//A view is a regular grid of tiles, as seen from a specific tile.
	//Because our tiles are non-euclidean, you may see tiles multiple times.

	class RayWalker {
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

	public:
		RayWalker(std::vector<std::vector<Tile*>>* field_);

		void reset(Tile* startingTile, int rot, int x, int y);

		bool moveTo(int x, int y);
	};

	uint8_t viewSize[2];
	std::vector<std::vector<Tile*>> grid;
	inline static Tile hiddenTile{};
	inline static Tile emptyTile{};

	RayWalker rayWalker{ &grid };

public:
	Tile* loc;
	int rot{ 0 };

	View(uint8_t width, uint8_t height, Tile* pointOfView);

	void render(std::ostream& target);

	//Borrowed and modified from http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html
	//TODO: Shoot more rays, presumably using the floating-point version.
	struct RaytraceParams {
		RayWalker rayWalker;
		
		//Source xy and dest xy.
		double sx{0}, sy{0}, dx, dy;
		
		//Callbacks, fired:
		const std::function<void(Tile*)> onAllTiles { nullptr }; //With each tile the ray passes through.
		const std::function<void(Tile*)> onLastTile { nullptr }; //With the final tile the ray passes through. May not be the target tile, might have hit something.
		const std::function<void(Tile*)> onTargetTile { nullptr }; //With the target tile the ray was directed at.
		
		//Debug.
		friend auto operator<<(std::ostream& os, RaytraceParams const& params) -> std::ostream&;
	};
	void raytrace(RayWalker& rayWalker, double sx, double sy, double dx, double dy);
	
	void moveCamera(int direction);
	
	void move(int direction);
	void turn(int delta);
};