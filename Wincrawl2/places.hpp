//Classes related to places, the tiles of the map itself.
#pragma once

#include <vector>
#include <random>

#include "color.hpp"
#include "things.hpp"

class Tile;

class Link {
	//A link is another tile we're linking to, and the direction we enter it by.

	Tile* tile_ = nullptr; //Link destination.
	uint8_t dir_ = 0; //Destination direction in which we emerge. Our entry in its links.

public:
	Tile* tile() const { return tile_; };
	uint8_t dir() const { return dir_; };

	void set(Tile* tile, uint8_t dir) {
		this->tile_ = tile;
		this->dir_ = dir;
	}

	void set(Link const& link) {
		this->tile_ = link.tile();
		this->dir_ = link.dir();
	}
	
	//todo: test this
	explicit operator bool () const {
		return this->tile_ != nullptr;
	}
};


class Tile {
	//A tile is a square place in a plane.

	inline static uint_fast16_t TotalTilesCreated;
	const uint_fast16_t id{ 0 };

public:
	//Let's define some geometry. For edges 0, 1, 2, 3, 4, 5:
	static constexpr uint8_t oppositeEdge[6]{ 2, 3, 0, 1, 5, 4 };
	static constexpr uint8_t rotateCW[6]{ 1, 2, 3, 0, 1, 3 }; //Rotate around the Z axis, ie, top-down.
	static constexpr uint8_t rotateCCW[6]{ 3, 0, 1, 2, 3, 1 }; //Going around a corner from the top will land you "facing" east or west, although it could just as easily be north and south as your rotation isn't tracked.

	//Since we are in a non-euclidean space here, N/E/S/W and Up/Down directions don't really make any sense.
	//However, if it helps, you can think of the links array as being such where N=0.
	Link links[6]{};
	uint8_t roomId{ 0 }; //0=uninitialized, 1=hidden, 2=empty, 9=hallway, 10≥rooms
	const char* glyph{ " " }; //String, 4 bytes + null terminator for utf8 astral plane characters.
	bool isOpaque{ false };
	Color bgColor{ 0, 0, 0 };
	Color fgColor{ 0, 0, 100 };
	std::vector<Entity<>*> occupants {};
	
	Tile() : id(TotalTilesCreated++) {}
	
	//Debugging functions.
	std::string getIDStr();
	std::string listLinks(int8_t hightlightIndex = -1);
	friend std::ostream& operator<<(std::ostream& os, Tile const& tile);

	void link(Tile* other, int8_t indexOut, int8_t indexIn = -1);
	void insert(Tile* newTile, int8_t indexOut, int8_t indexIn = -1);

	Link* getNextTile(int comingFrom, int pointingIn);
	Link* getNextTile(int directionIndex);

};


class Plane {
	//A plane is a collection of tiles, which are formed into rooms.
	
	inline static uint_fast16_t TotalPlanesCreated{ 0 };
	const uint_fast16_t id{ 0 };
	std::minstd_rand rng;
	int d(int max) {
		//Returns a number, 𝑛, such that 0 ≤ 𝑛 < max.
		return d(0, max);
	};
	int d(int min, int max) {
		//Returns a number, 𝑛, such that min ≤ 𝑛 < max.
		return std::uniform_int_distribution<int>{ min, max }(rng);
	};
	double d(double max) {
		//Returns a number, 𝑛, such that min ≤ 𝑛 < max.
		return d(0., max);
	};
	double d(double min, double max) {
		//Returns a number, 𝑛, such that min ≤ 𝑛 < max.
		return std::uniform_real_distribution{ min, max }(rng);
	};

	std::vector<Tile*> tiles; //List of all tiles we created.
	std::vector<Entity<>*> entities; //List of all entities we created. TODO: Track these as smart pointers, since we'll have many owners of indefinite lifetimes?
	
	struct RoomConnectionTile {
		Tile* tile;
		int8_t dir;
	};
	
	struct Room {
		Tile* seed;
		std::vector<RoomConnectionTile> connections; //TODO: Make this a vector of vectors, so we can have multi-tile wide connections.
	};
	std::vector<Room> rooms {};
	
	Room genSquareRoom(
		const uint8_t roomX, const uint8_t roomY, 
		const bool wrapX = false, const bool wrapY = false,
		const Color fg = Color{0,0,100}, //These can't be const, VS says no.
		const Color bg = Color{0,0,0},
		const uint8_t possibleDoors = 0b1111 //Bitfield, up/right/bottom/left like in CSS.
	);
	

public:
	Plane(std::minstd_rand rng, int numRooms);
	~Plane();

	friend std::ostream& operator<<(std::ostream& os, Plane const& plane);

	Tile* getStartingTile();
};