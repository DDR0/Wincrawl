//Classes related to places, the tiles of the map itself.
#include <array>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <functional>
#include <ranges>

#include "places.hpp"
#include "seq.hpp"
#include "vector_tools.hpp"


std::string Tile::listLinks(int8_t hightlightIndex) {
	std::stringstream out;
	out << "Tile " << this->getIDStr() << ":";
	for (int i = 0; i < 6; i++) {
		out << " "
			<< Color(this->links[i].tile() ? 0xF9343E : 0x20943E).fg()
			<< (i == hightlightIndex ? seq::bold : "")
			<< i << seq::reset
			<< (i == hightlightIndex ? "̲" : "");
	}
	return out.str();
}

std::ostream& operator<<(std::ostream& os, Tile const& tile) {
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


void Tile::link(Tile* other, int8_t indexOut, int8_t indexIn) {
	//Connect two tiles together, where both connections are free.

	if (indexIn == -1) {
		indexIn = oppositeEdge[indexOut];
	}

	assert(other);
	assert(indexOut >= 0 && indexOut < 6); //Direction index out of this tile must be specified.
	assert(indexIn >= 0 && indexIn < 6);

	//Defined in Release mode by VS. We assume if all tiles linked successfully in debug mode, they will be fine in release mode.
#ifndef NDEBUG
	//Don't allow resetting tile links here? Use insert for that. (Prevent one-way links. Jumps in perspective are not desired, you can always go back.)
	//This is more than just an assert because knowing **how** we messed up the linking is very helpful for development.
	const bool badLink{
		this->links[indexOut].tile() or
		other->links[indexIn].tile()
	};
	if (badLink) {
		std::cerr << "\nTile Link Error\n";

		std::cerr << "Tile " << this->getIDStr() << " indexOut " << (int)indexOut;
		if (this->links[indexOut].tile()) {
			std::cerr << " already points to tile "
				<< this->links[indexOut].tile()->getIDStr() << ".\n";
		}
		else {
			std::cerr << " clear.\n";
		}
		std::cerr << (this->listLinks(indexOut)) << "\n";

		std::cerr << "Tile " << other->getIDStr() << " indexIn " << (int)indexIn;
		if (other->links[indexIn].tile()) {
			std::cerr << " already points to tile "
				<< other->links[indexIn].tile()->getIDStr() << ".\n";
		}
		else {
			std::cerr << " clear.\n";
		}
		std::cerr << other->listLinks(indexIn) << "\n\n";

		assert(false);
	}
#endif

	other->links[indexIn].set(this, indexOut);
	this->links[indexOut].set(other, indexIn);
}

void Tile::insert(Tile* newTile, int8_t indexOut, int8_t indexIn) {
	//Put a tile between two connected tiles. (Neither of the tiles otherwise move.)

	if (indexIn == -1) {
		indexIn = oppositeEdge[indexOut];
	}

	assert(newTile);
	assert(indexOut != indexIn); //Indices in are for the internal tile, since we know the outter tile indices this time.
	assert(indexOut >= 0 && indexOut < 6); //Direction index out of this tile must be specified.
	assert(indexIn >= 0 && indexIn < 6);
	assert(this->links[indexOut].tile()); //Don't allow resetting tile links here? Use insert for that. (Prevent one-way links. Jumps in perspective are not desired, you can always go back.)
	assert(!newTile->links[indexIn].tile());
	assert(!newTile->links[oppositeEdge[indexIn]].tile());

	Link& outbound{ this->links[indexOut] };
	Link& inbound{ outbound.tile()->links[outbound.dir()] };

	//Copy links to inserted tile's links.
	newTile->links[indexOut].set(outbound);
	newTile->links[indexIn].set(inbound);

	//Update source and destTile tile's links.
	outbound.set(newTile, indexIn);
	inbound.set(newTile, indexOut);
}

Link* Tile::getNextTile(int comingFrom, int pointingIn) {
	//comingFrom is an absolute, a direction index.
	//pointingIn is a relative direction index. (a rotation around z in quarters)
	assert(0 <= comingFrom && comingFrom < 6);
	assert(-4 < pointingIn && pointingIn < 4);
	switch (pointingIn) {
	case +0: return &links[oppositeEdge[comingFrom]];
	case -1:
	case +3: return &links[rotateCCW[comingFrom]];
	case -3:
	case +1: return &links[rotateCW[comingFrom]];
	case -2:
	case +2: return &links[comingFrom];
	default:
		std::cerr << "Expected pointingIn to be in range [-3,3]; was " << pointingIn << "\n";
		assert(false);
		return nullptr;
	}
}

Link* Tile::getNextTile(int directionIndex) {
	assert(0 <= directionIndex && directionIndex < 6);
	return &(links[directionIndex]);
}

std::string Tile::getIDStr() {
	constexpr int idDigitLength = 4;
	auto buf = std::make_unique<char[]>(idDigitLength);
	std::snprintf(buf.get(), idDigitLength, "%0*lu", idDigitLength - 1, id);
	return std::string(buf.get());
}



Plane::RoomConnectionTile::RoomConnectionTile(Tile* tile_, int8_t dir_) : tile(tile_), dir(dir_) {
	assert(("Cannot create doorway: Direction does not point to a wall.", !tile->links[dir]));
}

Plane::Room Plane::genSquareRoom(
	const uint_fast8_t roomX, const uint_fast8_t roomY,
	const bool wrapX, const bool wrapY,
	const Color fg, const Color bg,
	const uint_fast8_t possibleDoors
) {
	std::vector<std::vector<Tile*>> room{ roomX, {roomY, nullptr} };

	for (uint8_t x = 0; x < roomX; x++) {
		for (uint8_t y = 0; y < roomY; y++) {
			Tile* tile{ newOwnedTile() };
			room[x][y] = tile;

			tile->roomId = 10;
			tile->glyph = (x + y) % 2 ? "," : ".";
			tile->fgColor = fg;
			tile->bgColor = bg;
		}
	}

	for (uint8_t x = 0; x < roomX - (!wrapX); x++) { //-0 to loop, -1 to not
		for (uint8_t y = 0; y < roomY; y++) {
			room[x][y]->link(room[(x + 1) % roomX][y], 1);
		}
	}

	for (uint8_t x = 0; x < roomX; x++) {
		for (uint8_t y = 0; y < roomY - (!wrapY); y++) { //-0 to loop, -1 to not
			room[x][y]->link(room[x][(y + 1) % roomY], 2);
		}
	}

	auto doors = possibleDoors;
	std::vector<RoomConnectionTile> connections{};
	if (!wrapX) {
		if (doors & 0b0100) connections.emplace_back(room[roomX - 1][roomY / 2], 1);
		if (doors & 0b0001) connections.emplace_back(room[0][roomY / 2], 3);
	}
	if (!wrapY) {
		if (doors & 0b1000) connections.emplace_back(room[roomX / 2][0], 0);
		if (doors & 0b0010) connections.emplace_back(room[roomX / 2][roomY - 1], 2);
	}

	return Room{ room[roomX / 2][roomY / 2], connections };
}

Plane::Room Plane::genHallway(
	const uint_fast8_t length,
	const Plane::genHallwayStyle style
) {
	return genHallway(length, 1, style);
}
Plane::Room Plane::genHallway(
	const uint_fast8_t length, const uint_fast8_t width,
	const Plane::genHallwayStyle style
) {
	//std::cerr << "Creating " << (int)length << "x" << (int)width << " hallway with style " << (int)style << ".\n";

	int totalHallTiles = static_cast<size_t>(length) * static_cast<size_t>(width);
	std::vector<Tile*> hall{};
	hall.reserve(totalHallTiles);

	//genHallwayStyle
	constexpr int CURVE_TYPES = 7;
	const uint_fast8_t modifier = d(CURVE_TYPES);
	static const std::array<std::function<int8_t(int, int)>, 5> curvature{
		[](int tileNum, int totalTiles) { //straight
			return 1;
		},
		[](int tileNum, int totalTiles) { //zig-zag
			return std::array<int8_t,3>{1,2,1}[static_cast<size_t>(tileNum / 1.0 / totalTiles * 3.0)];
		},
		[modifier](int tileNum, int totalTiles) { //spiralCW
			return std::array<std::array<int8_t,6>, CURVE_TYPES>{
				std::array<int8_t, 6>{1, 2, 1, 2, 1, 2}, //small spiral
				std::array<int8_t, 6>{1, 1, 2, 1, 1, 2}, //med spiral
				std::array<int8_t, 6>{1, 1, 2, 1, 1, 2}, //med spiral
				std::array<int8_t, 6>{1, 1, 1, 1, 1, 2}, //large spiral
				std::array<int8_t, 6>{1, 1, 1, 1, 2, 2}, //staircase
				std::array<int8_t, 6>{1, 1, 1, 1, 2, 2}, //staircase
				std::array<int8_t, 6>{1, 2, 2, 1, 1, 1}, //staircase
			}[modifier][tileNum % 6];
		},
		[modifier](int tileNum, int totalTiles) { //spiralCCW
			return std::array<std::array<int8_t,6>, CURVE_TYPES>{
				std::array<int8_t, 6>{1, 0, 1, 0, 1, 0}, //small spiral
				std::array<int8_t, 6>{1, 1, 0, 1, 1, 0}, //med spiral
				std::array<int8_t, 6>{1, 1, 0, 1, 1, 0}, //med spiral
				std::array<int8_t, 6>{1, 1, 1, 1, 1, 0}, //large spiral
				std::array<int8_t, 6>{1, 1, 1, 1, 0, 0}, //staircase
				std::array<int8_t, 6>{1, 0, 0, 1, 1, 1}, //staircase
			}[modifier][tileNum % 6];
		},
		[this](int tileNum, int totalTiles) { //irregular
			return d(2);
		}
	};
	static_assert(curvature.size() == static_cast<int>(genHallwayStyle::COUNT));

	hall.emplace_back(newOwnedTile());
	for (int i : std::views::iota(1,totalHallTiles)) {
		Tile* head{ hall.back() };
		Tile* tile{ hall.emplace_back(newOwnedTile()) };

		auto indexOut = curvature[static_cast<int>(style)](i,totalHallTiles);
		head->link(tile, indexOut, 3);
	}

	std::vector<RoomConnectionTile> connections{};
	connections.emplace_back(hall.front(), 3);
	connections.emplace_back(hall.back(), 1);

	return Room{ hall.at(totalHallTiles/2), connections };
}

Plane::Plane(std::minstd_rand rng_, int numRooms)
	: id(TotalPlanesCreated++), rng(rng_)
{

	//auto choose10 { std::bind(std::uniform_int_distribution<int>{ 0, 10 }, rng) };
	//std::cout << "rng says " << d(20) << "\n";

	tiles.reserve(512);
	rooms.reserve(numRooms);

	//First, generate a number of rooms.
	for ([[maybe_unused]] auto _ : std::views::iota(0, numRooms)) {
		rooms.emplace_back(genSquareRoom(
			d(2, 5) + d(2, 5), d(2, 5) + d(2, 5),
			!d(4), false,
			Color{ d(30.,115.), d(58.,  70.), d(35., 50.) },
			Color{ d(30.,115.), d(70., 100.), d(0.,  6.) }
		));

		//Randomise each room's connections.
		std::shuffle(rooms.back().connections.begin(), rooms.back().connections.end(), rng);
	}
	
	//Second, link all the rooms up so none are orphaned.
	for (size_t i : std::views::iota(1, static_cast<int>(rooms.size()))) {
		auto roomAConns { &rooms.at(i - 1).connections };
		auto roomBConns { &rooms.at(i - 0).connections };
		auto hallConns{
			(
				d(2)
					? genHallway(1, genHallwayStyle::straight)
					: genHallway((d(4, 24) + d(4, 24)) / 2,
						static_cast<genHallwayStyle>(d(static_cast<int>(genHallwayStyle::COUNT))))
			).connections
		};

		assert(("Map gen error: Room A has no connections.", roomAConns->size()));
		assert(("Map gen error: Room B has no connections.", roomBConns->size()));
		assert(("Map gen error: Connecting hall has no connections.", hallConns.size() >= 2));

		RoomConnectionTile roomA{ roomAConns->back() };
		RoomConnectionTile roomB{ roomBConns->back() };
		RoomConnectionTile doorA{ hallConns.at(0) };
		RoomConnectionTile doorB{ hallConns.at(1) };

		roomA.tile->link(doorA.tile, roomA.dir, doorA.dir);
		roomB.tile->link(doorB.tile, roomB.dir, doorB.dir);

		//Consume used doors.
		roomAConns->pop_back();
		roomBConns->pop_back();
	}
}

Plane::~Plane() {
	for (auto tile : tiles) { delete tile; }
	for (auto entity : entities) { delete entity; }
}

std::ostream& operator<<(std::ostream& os, Plane const& plane) {
	os << "Plane " << plane.id << ":\n\t";

	size_t tcount{};
	for (auto tile : plane.tiles) {
		os << *tile << ", ";
		if (not (++tcount % 5) && tcount != plane.tiles.size()) {
			os << "\n\t";
		}
	}

	return os << "\n";
}

Tile* Plane::getStartingTile() {
	return rooms.at(0).seed;
}