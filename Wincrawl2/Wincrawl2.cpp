#include <iostream>
#include <sstream>
#include <cassert>
#include <vector>

#ifdef _MSC_VER
#include <windows.h>
#endif

enum relativeDirection { dir_forward, dir_backward, dir_left, dir_right, dir_up, dir_down };



class Tile {
	//A tile is a square place in a plane.

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
	};

	inline static uint_fast16_t TotalTilesCreated;
	const uint_fast16_t id{ 0 };

	//Now, let's define some geometry. For edges 0, 1, 2, 3, 4, 5:
	static constexpr uint8_t oppositeEdge[6]{ 2, 3, 0, 1, 5, 4 };
	static constexpr uint8_t rotateCW[6]{ 1, 2, 3, 0, 4, 5 }; //Rotate around the Z axis, ie, top-down.
	static constexpr uint8_t rotateCCW[6]{ 3, 0, 1, 2, 4, 5 };

public:
	//Since we are in a non-euclidean space here, N/E/S/W and Up/Down directions don't really make any sense.
	//However, if it helps, you can think of the links array as being such where N=0.
	Link links[6]{};
	uint8_t roomId{ 0 };
	uint8_t glyph{ 0 };
	bool isSolid{ false };

	Tile() : id(TotalTilesCreated++) {}

	friend auto operator<<(std::ostream& os, Tile const& tile) -> std::ostream& {
		os << "Tile " << tile.id << " (";

		bool hasNeighbours = false;
		for (int i = 0; i < 6; i++) {
			if (not tile.links[i].tile()) continue;
			if (hasNeighbours) os << " ";
			os << i << "→" << tile.links[i].tile()->id;
			hasNeighbours = true;
		}
		if (!hasNeighbours) os << "isolated";

		return os << ")";
	}


	void link(Tile* other, int8_t indexOut, int8_t indexIn = -1) {
		//Connect two tiles together, where both connections are free.

		if (indexIn == -1) {
			indexIn = oppositeEdge[indexOut];
		}

		assert(other);
		assert(indexOut >= 0 && indexOut < 6); //Direction index out of this tile must be specified.
		assert(indexIn >= 0 && indexIn < 6);
		assert(!this->links[indexOut].tile()); //Don't allow resetting tile links here? Use insert for that. (Prevent one-way links. Jumps in perspective are not desired, you can always go back.)
		assert(!other->links[indexIn].tile());

		other->links[indexIn].set(this, indexOut);
		this->links[indexOut].set(other, indexIn);
	}

	void insert(Tile* other, int8_t indexOut, int8_t indexIn = -1) {
		//Put a tile between two connected tiles. (Neither of the tiles otherwise move.)

		if (indexIn == -1) {
			indexIn = oppositeEdge[indexOut];
		}

		assert(other);
		assert(indexOut != indexIn); //Indices in are for the internal tile, since we know the outter tile indices this time.
		assert(indexOut >= 0 && indexOut < 6); //Direction index out of this tile must be specified.
		assert(indexIn >= 0 && indexIn < 6);
		auto opposite{ oppositeEdge[indexOut] };
		assert(this->links[indexOut].tile()); //Don't allow resetting tile links here? Use insert for that. (Prevent one-way links. Jumps in perspective are not desired, you can always go back.)
		assert(!other->links[indexIn].tile());
		assert(!other->links[oppositeEdge[indexIn]].tile());

		Tile* source{ this };
		Link& outbound{ source->links[indexOut] };
		Tile* dest{ source->links[indexOut].tile() };
		Link& inbound{ dest->links[outbound.dir()] };

		//Copy links to inserted tile's links.
		other->links[indexOut].set(outbound);
		other->links[indexIn].set(inbound);

		//Update source and dest tile's links.
		outbound.set(other, outbound.dir());
		inbound.set(other, inbound.dir());
	}
};


class Plane {
	//A plane is a collection of tiles, which are formed into rooms.

	inline static uint_fast16_t TotalPlanesCreated{ 0 };
	const uint_fast16_t id{ 0 };

	std::vector<Tile*> tiles; //List of all tiles we created.
	std::vector<Tile*> roomSeeds;

public:
	Plane(uint16_t rooms)
		: id(TotalPlanesCreated++)
	{
		constexpr uint8_t roomX{ 5 };
		constexpr uint8_t roomY{ 7 };
		Tile* room[roomX][roomY] = {};

		for (uint8_t x = 0; x < roomX; x++) {
			for (uint8_t y = 0; y < roomY; y++) {
				Tile* tile{ new Tile() };
				tile->roomId = 1;
				tile->glyph = (x + y) % 2 ? ',' : '.';

				tiles.push_back(tile);
				room[x][y] = tile;
			}
		}

		for (uint8_t x = 0; x < roomX; x++) {
			for (uint8_t y = 0; y < roomY-1; y++) {
				room[x][y]->link(room[(x + 1) % roomX][y], 1);
				room[x][y]->link(room[x][y + 1], 2);
			}
		}

		//A statue of a person, let's say.
		roomSeeds.push_back(room[2][3]);
		room[2][3]->glyph = '@';
		room[2][3]->isSolid = true;
	}

	~Plane() {
		for (auto tile : tiles) {
			delete tile;
		}
	}

	friend auto operator<<(std::ostream& os, Plane const& plane) -> std::ostream& {
		os << "Plane " << plane.id << ":\n\t";

		int tcount{};
		for (auto tile : plane.tiles) {
			os << *tile << ", ";
			if (not (++tcount % 5) && tcount != (int) plane.tiles.size()) {
				os << "\n\t";
			}
		}

		return os << "\n";
	}

	Tile* getStartingTile() {
		return roomSeeds[0];
	}
};



class View {
	//A view is a regular grid of tiles, as seen from a specific tile.
	//Because our tiles are non-euclidean, you may see tiles multiple times.

public:
	Tile* loc{};

	void render(std::ostream& target) {
		assert(loc); //If no location is defined, fail.
		target << loc->glyph;
	}
};


int main() {
	using namespace std;
	
	#ifdef _MSC_VER
	SetConsoleOutputCP(CP_UTF8);
	#endif

	cout << "⌛ Generating...\n";

	Tile tile0{};
	cout << "tiles: " << tile0 << "\n";

	Tile tile1{};
	tile0.link(&tile1, 1, 3);
	cout << "tiles: " << tile0 << " " << tile1 << "\n";

	Tile tile2{};
	tile0.insert(&tile2, 1, 2);
	cout << "tiles: " << tile0 << " " << tile1 << " " << tile2 << "\n";

	Plane plane0{ 4 };
	
	cout << "Done!";
	
	View view{};
	view.loc = plane0.getStartingTile();
	view.render(cout);
	
	return 0;

	string command;
	do {
		getline(cin, command);
		cout << "> " << command << "\n";
	} while (command != "q");
}