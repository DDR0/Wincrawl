#include <iostream>
#include <array>
#include <sstream>
#include <string>
#include <cassert>

#include <windows.h>

enum relativeDirection { dir_forward, dir_backward, dir_left, dir_right, dir_up, dir_down};

class Tile;
class Link {
	//A link is another tile we're linking to, and the direction we enter it by.

private:
	Tile* tile_ = nullptr; //Link destination.
	uint8_t dir_ = 0; //Destination direction, our entry in its links.

public:
	Tile* tile() const { return tile_; };
	uint8_t dir() const { return dir_; };

	void set(Tile* tile, uint8_t dir) {
		this->tile_ = tile;
		this->dir_ = dir;
	}
};

class Tile
{
private:
	static uint_fast16_t TotalTilesCreated;
	const uint_fast16_t id{ 0 };

	//Now, let's define some geometry. For edges 0, 1, 2, 3, 4, 5:
	static constexpr uint8_t oppositeEdge[6]{ 2, 3, 0, 1, 5, 4 };
	static constexpr uint8_t rotateCW[6]{ 1, 2, 3, 0, 4, 5 }; //Rotate around the Z axis, ie, top-down.
	static constexpr uint8_t rotateCCW[6]{ 3, 0, 1, 2, 4, 5 };

public:
	//Since we are in a non-euclidean space here, N/E/S/W and Up/Down directions don't really make any sense.
	//However, if it helps, you can think of the links array as being such where N=0.
	Link links[6] = {};
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


	void link(Tile* other, const int8_t indexOut, const int8_t indexIn = -1) {
		assert(other);
		assert(indexOut >= 0 && indexOut < 6); //Direction index out of this tile must be specified.
		assert(indexIn >= -1 && indexIn < 6); //-1 for "opposite of direction index in".
		assert(!this->links[indexOut].tile()); //Don't allow resetting tile links here? Use insert for that. (Prevent one-way links. Jumps in perspective are not desired, you can always go back.)
		assert(!other->links[indexIn].tile());

		this->links[indexOut].set(other, indexIn);
		other->links[indexIn].set(this, indexOut);
	}
};
uint_fast16_t Tile::TotalTilesCreated{ 0 };




int main() {
	using namespace std;

	SetConsoleOutputCP(CP_UTF8);

	cout << "⌛ Generating...\n";
	Tile tile1 = Tile();
	Tile tile2 = Tile();
	cout << "tiles: " << tile1 << " " << tile2 << "\n";
	tile1.link(&tile2, 1, 3);
	cout << "tiles: " << tile1 << " " << tile2 << "\n";

	return 0;

	string command;
	do {
		getline(cin, command);
		cout << "> " << command << "\n";
	} while (command != "q");
}