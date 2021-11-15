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


struct painter {
	int x{ 0 };
};


std::string Tile::listLinks(int8_t hightlightIndex) {
	std::stringstream out;
	out << "Tile " << this->getIDStr() << ":";
	for (int i = 0; i < 6; i++) {
		out << " "
			<< Color(this->links[i].tile() ? 0xF9343EFF : 0x20943EFF).fg()
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
	
	tiles.reserve(6);
	tiles.push_back(tile_);
}

bool Plane::allRoomConnectionsAreFree(std::vector<Room> rooms) {
	for (auto& room : rooms) {
		for (auto& connection : room.connections) {
			if (connection.tile->links[connection.dir]) {
				return false;
			}
		}
	}
	return true;
};

Plane::Room Plane::genSquareRoom(
	const uint_fast8_t roomX, const uint_fast8_t roomY,
	const bool wrapX, const bool wrapY,
	const Color fg, const Color bg,
	const uint_fast8_t possibleDoors
) {
	std::vector<std::vector<Tile*>> room{ roomX, {roomY, nullptr} };

	for (uint_fast8_t x = 0; x < roomX; x++) {
		for (uint_fast8_t y = 0; y < roomY; y++) {
			Tile* tile{ newOwnedTile() };
			room[x][y] = tile;

			tile->roomId = 10;
			tile->glyph = &" \0 \0.\0,"[d(4) * 2];
			tile->fgColor = fg;
			tile->bgColor = bg;
		}
	}

	for (uint_fast8_t x = 0; x < roomX - (!wrapX); x++) { //-0 to loop, -1 to not
		for (uint_fast8_t y = 0; y < roomY; y++) {
			room[x][y]->link(room[(x + 1) % roomX][y], 1);
		}
	}

	for (uint_fast8_t x = 0; x < roomX; x++) {
		for (uint_fast8_t y = 0; y < roomY - (!wrapY); y++) { //-0 to loop, -1 to not
			room[x][y]->link(room[x][(y + 1) % roomY], 2);
		}
	}
	
	auto doors = possibleDoors;
	std::vector<RoomConnectionTile> connections{};

	const size_t top         = 0;
	const size_t left        = 0;
	const size_t bottom      = roomY - 1;
	const size_t right       = roomX - 1;
	const size_t halfWidth   = roomX / 2;
	const size_t halfHeight  = roomY / 2;
	const size_t leftThird   = static_cast<size_t>(round(roomX / 3.0));
	const size_t rightThird  = static_cast<size_t>(round(roomX / 1.5));
	const size_t topThird    = static_cast<size_t>(round(roomY / 3.0));
	const size_t bottomThird = static_cast<size_t>(round(roomY / 1.5));
	
	if(wrapX && wrapY) { //No walls, no doors.
	}
	else if (!wrapX && !wrapY) { //Square room, put one door in each wall.
		if (doors & 0b0001) connections.emplace_back(room[halfWidth][top       ], 0);
		if (doors & 0b0010) connections.emplace_back(room[right    ][halfHeight], 1);
		if (doors & 0b0100) connections.emplace_back(room[halfWidth][bottom    ], 2);
		if (doors & 0b1000) connections.emplace_back(room[left     ][halfHeight], 3);
	}
	else if (!wrapX) {
		if (roomY <= 3) {
			if (doors & 0b0001) connections.emplace_back(room[left      ][halfHeight ], 3);
			if (doors & 0b0100) connections.emplace_back(room[right     ][halfHeight ], 1);
		} else {
			if (doors & 0b0001) connections.emplace_back(room[left      ][topThird   ], 3);
			if (doors & 0b0010) connections.emplace_back(room[left      ][bottomThird], 3);
			if (doors & 0b0100) connections.emplace_back(room[right     ][topThird   ], 1);
			if (doors & 0b1000) connections.emplace_back(room[right     ][bottomThird], 1);
		}
	}
	else if (!wrapY) {
		if (roomX <= 3) {
			if (doors & 0b0010) connections.emplace_back(room[halfWidth ][bottom     ], 2);
			if (doors & 0b1000) connections.emplace_back(room[halfWidth ][top        ], 0);
		} else {
			if (doors & 0b0001) connections.emplace_back(room[leftThird ][bottom     ], 2);
			if (doors & 0b0010) connections.emplace_back(room[rightThird][bottom     ], 2);
			if (doors & 0b0100) connections.emplace_back(room[leftThird ][top        ], 0);
			if (doors & 0b1000) connections.emplace_back(room[rightThird][top        ], 0);
		}
	}
	else {
		assert(!"Logic error.");
	}

	return Room{ room[roomX / 2][roomY / 2], connections };
}

Plane::Room Plane::genConicalRoom(
	const int height,
	const Color fg, const Color bg,
	const uint_least8_t possibleDoors
) {
	//Assemble a cone from an L-shape, gluing together the concave edges.
	// █  ← top
	// ██ ← bottom
	std::vector<std::vector<Tile*>> top{ static_cast<size_t>(height), {static_cast<size_t>(height), nullptr} };
	std::vector<std::vector<Tile*>> bottom{ static_cast<size_t>(height)*2, {static_cast<size_t>(height), nullptr} };
	
	const auto iota { std::views::iota };

	//Work around some math warnings regarding math on lesser integer width variables, even though it's fine in this case.
	//iota() requires integers, array deindexing requires size_ts, so we do want both types.
	const auto height_{ static_cast<size_t>(height) };
	
	//Gen top tiles.
	for (int x : iota(0, height)) {
		for (int y : iota(0, height)) {
			Tile* tile{ newOwnedTile() };
			top[x][y] = tile;
			
			tile->roomId = 10;
			tile->glyph = &" \0 \0.\0,"[d(4) * 2];
			tile->fgColor = fg;
			tile->bgColor = bg;
		}
	}

	//Gen bottom tiles. (Twice as wide as top, since the top will mesh with the side.)
	for (int x : iota(0, height * 2)) {
		for (int y : iota(0, height)) {
			Tile* tile{ newOwnedTile() };
			bottom[x][y] = tile;

			tile->roomId = 10;
			tile->glyph = &" \0 \0.\0,"[d(4) * 2];
			tile->fgColor = fg;
			tile->bgColor = bg;
		}
	}
	
	//Link top tiles horizontally.
	for (size_t x : iota(0, height-1)) {
		for (size_t y : iota(0, height)) {
			top[x][y]->link(top[x+1][y], 1);
		}
	}
	
	//Link top tiles vertically.
	for (size_t x : iota(0, height)) {
		for (size_t y : iota(0, height-1)) {
			top[x][y]->link(top[x][y+1], 2);
		}
	}

	//Link bottom tiles horizontally.
	for (size_t x : iota(0, height*2 - 1)) {
		for (size_t y : iota(0, height)) {
			bottom[x][y]->link(bottom[x + 1][y], 1);
		}
	}

	//Link bottom tiles vertically.
	for (size_t x : iota(0, height*2)) {
		for (size_t y : iota(0, height - 1)) {
			bottom[x][y]->link(bottom[x][y + 1], 2);
		}
	}
	
	//Link first half of the top of bottom tiles with the bottom of top tiles.
	for (size_t i : iota(0, height)) {
		top[i][height_-1]->link(bottom[i][0], 2);
	}
	
	//Link second half of the top of bottom tiles with the right side of top tiles.
	for (size_t i : iota(0, height)) {
		top[height_-1][i]->link(bottom[height_*2-1-i][0], 1, 0);
	}

	auto doors = possibleDoors;
	std::vector<RoomConnectionTile> connections{}; //Offset slightly CCW since room is always an even number of tiles wide.
	if (doors & 0b001) connections.emplace_back(top[height_-1][0], 0);
	if (doors & 0b010) connections.emplace_back(bottom[0][0], 3);
	if (doors & 0b100) connections.emplace_back(bottom[height_+1][height_-1], 2);

	return Room{ top[height_-1][height_-1], connections };
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
	constexpr int CURVE_TYPES { 7 };
	const int zigZagRotation { d(2) ? -1 : 1 };
	const int zigZagType { d(2) };
	const int curveIndex { d(CURVE_TYPES) };
	static const std::array<std::function<int8_t(size_t, size_t)>, 5> curvature{
		[](size_t, size_t) { //straight
			return 1;
		},
		[&zigZagRotation, &zigZagType](size_t tileNum, size_t totalTiles) { //zig-zag, must capture closures by reference or they never change across invocations.
			const std::array<size_t, 2> patSize {2,3};
			const std::array<std::array<int, 3>, 2> pattern {{
				{0,1},
				{0,1,0},
			}};
			auto currAbsoluteDirection = pattern[zigZagType][static_cast<size_t>((tileNum+0) / 1.0 / (totalTiles+1) * patSize[zigZagType])];
			auto nextAbsoluteDirection = pattern[zigZagType][static_cast<size_t>((tileNum+1) / 1.0 / (totalTiles+1) * patSize[zigZagType])];
			//std::cerr << "zzdir: tile "
			//	<< tileNum << "/" << totalTiles << " "
			//	<< 1 + (currAbsoluteDirection - nextAbsoluteDirection) * zigZagRotation
			//	<< " from " << (int)currAbsoluteDirection << "-" << (int)nextAbsoluteDirection
			//	<< ", rot " << (int)zigZagRotation << ", type " << (int)zigZagType
			//	<< "\n";
			return 1 + (currAbsoluteDirection - nextAbsoluteDirection) * zigZagRotation;
		},
		[&curveIndex](size_t tileNum, size_t) { //spiralCW
			return std::array<std::array<int8_t,6>, CURVE_TYPES>{
				std::array<int8_t, 6>{1, 2, 1, 2, 1, 2}, //small spiral
				std::array<int8_t, 6>{1, 1, 2, 1, 1, 2}, //med spiral
				std::array<int8_t, 6>{1, 1, 2, 1, 1, 2}, //med spiral
				std::array<int8_t, 6>{1, 1, 1, 1, 1, 2}, //large spiral
				std::array<int8_t, 6>{1, 1, 1, 1, 2, 2}, //staircase
				std::array<int8_t, 6>{1, 1, 1, 1, 2, 2}, //staircase
				std::array<int8_t, 6>{1, 2, 2, 1, 1, 1}, //staircase
			}[curveIndex][tileNum % 6];
		},
		[&curveIndex](size_t tileNum, size_t) { //spiralCCW
			return std::array<std::array<int8_t,6>, CURVE_TYPES>{
				std::array<int8_t, 6>{1, 0, 1, 0, 1, 0}, //small spiral
				std::array<int8_t, 6>{1, 1, 0, 1, 1, 0}, //med spiral
				std::array<int8_t, 6>{1, 1, 0, 1, 1, 0}, //med spiral
				std::array<int8_t, 6>{1, 1, 1, 1, 1, 0}, //large spiral
				std::array<int8_t, 6>{1, 1, 1, 1, 0, 0}, //staircase
				std::array<int8_t, 6>{1, 1, 1, 1, 0, 0}, //staircase
				std::array<int8_t, 6>{1, 0, 0, 1, 1, 1}, //staircase
			}[curveIndex][tileNum % 6];
		},
		[this](size_t, size_t) { //irregular
			return d(2);
		}
	};
	static_assert(curvature.size() == static_cast<int>(genHallwayStyle::COUNT));

	//std::cerr << "Starting hall: (len " << totalHallTiles << ")\n";
	hall.emplace_back(newOwnedTile());
	for (int i : std::views::iota(1,totalHallTiles)) {
		Tile* head{ hall.back() };
		Tile* tile{ hall.emplace_back(newOwnedTile()) };
		
		auto indexOut = curvature[static_cast<int>(style)](i,totalHallTiles);
		//std::cerr << "Style " << (int)style << ": " << (int)indexOut << " at step " << i << "/" << totalHallTiles << "\n";
		head->link(tile, indexOut, 3);
	}

	std::vector<RoomConnectionTile> connections{};
	connections.emplace_back(hall.front(), 3);
	connections.emplace_back(hall.back(), 1);

	return Room{ hall.at(totalHallTiles/2), connections };
}

void Plane::linkConnectionsWithHallway(auto& roomAConns, auto& roomBConns) {
	auto hallConns{
		(
			d(2)
				? genHallway(1, genHallwayStyle::straight)
				: genHallway((d(4, 9) + d(4, 9)) / 2,
					static_cast<genHallwayStyle>(d(static_cast<int>(genHallwayStyle::COUNT))))
		).connections
	};

	assert(("Map gen error: Room A has no connections.", roomAConns.size()));
	assert(("Map gen error: Room B has no connections.", roomBConns.size()));
	assert(("Map gen error: Connecting hall has no connections.", hallConns.size() >= 2));
	if(&roomAConns == &roomBConns) {
		assert((
			"Map gen error: Room doesn't have enough open connections to connect to itself.",
			roomAConns.size() >= 2
		));
	}

	RoomConnectionTile roomA{ roomAConns.back() };
	roomAConns.pop_back(); //Consume used doors.
	RoomConnectionTile roomB{ roomBConns.back() };
	roomBConns.pop_back();
	
	RoomConnectionTile doorA{ hallConns.at(0) };
	RoomConnectionTile doorB{ hallConns.at(1) };

	roomA.tile->link(doorA.tile, roomA.dir, doorA.dir);
	roomB.tile->link(doorB.tile, roomB.dir, doorB.dir);

}

Plane::Plane(std::minstd_rand rng_, int numRooms)
	: id(TotalPlanesCreated++), rng(rng_)
{

	//auto choose10 { std::bind(std::uniform_int_distribution<int>{ 0, 10 }, rng) };
	//std::cout << "rng says " << d(20) << "\n";

	tiles.reserve(512);
	rooms.reserve(numRooms);
	
	const auto iota { std::views::iota };

	//First, generate a number of rooms.
	for ([[maybe_unused]] auto _ : iota(0, numRooms)) {
		switch (d(3)) {
		case 0:
		case 1:
			rooms.emplace_back(genSquareRoom(
				d(2, 5) + d(2, 5), d(2, 5) + d(2, 5),
				!d(4), false,
				Color{ d(30.,115.), d(58.,  70.), d(35., 50.) },
				Color{ d(30.,115.), d(70., 100.), d(0.,  6.) }
			));
			break;
		case 2:
			rooms.emplace_back(genConicalRoom(
				d(1, 3) + d(1, 3),
				Color{ d(30.,115.), d(58.,  70.), d(45., 60.) },
				Color{ d(30.,115.), d(70., 100.), d(1.,  7.) },
				0b111
			));
		break;
		default:
			assert(("Logic error, invalid room type.", false));
		}

		//Randomise each room's connections.
		std::shuffle(rooms.back().connections.begin(), rooms.back().connections.end(), rng);
	}
	
	assert(allRoomConnectionsAreFree(rooms));
	
	//Second, link all the rooms up so we don't get stuck.
	for (size_t i : iota(1, static_cast<int>(rooms.size()))) {
		linkConnectionsWithHallway(
			rooms.at(i - 1).connections, 
			rooms.at(i - 0).connections
		);
	}
	
	assert(allRoomConnectionsAreFree(rooms));
	
	//Third, link up a few more rooms so it's not just a linear labyrinth. (We have consumed over half our linkage opportunities at this point.)
	const int extraConnections = static_cast<int>(rooms.size()/4);
	for (int connectionNumber : std::views::iota(0, extraConnections)) {
		//Replace the following with https://github.com/liamwhite/format-preserving/blob/7da768a732b8bc79d24a5d195a61040271014321/src/main.rs#L3-L58 at some point, it does what we want much better and without the O(n) memory cost.
		//Constexpr-size only. std::shuffle_order_engine<std::linear_congruential_engine<uint_fast8_t, 1, 1, 10>, 10> order { std::uniform_int_distribution<uint_fast8_t>{ 0, 10-1 }(rng) };
		
		Room* connect[2] { nullptr };
		for (int i : iota(0,2)) {
			for ([[maybe_unused]] auto attempt : iota(0,10)) {
				Room* room { &rooms.at(d(static_cast<int>(rooms.size()))) };
				assert(room); //Silence 'could be null' warning.
				
				if (connect[0] == room && room->connections.size() < 2) {
					//Reject the second room if it's the same as the first room and doesn't have enough connections to connect to itself.
				}
				else if (room->connections.size() < 1) {
					//Reject a room if it doesn't have any connections left.
				} 
				else {
					connect[i] = room;
					break;
				}
			}
		}
		
		if (!connect[0] || !connect[1]) {
			std::cerr << "Warning: Could not find spare doorway for room interlink " << connectionNumber << "/" << extraConnections << ".\nWarning: Skipping further interlink steps.\n";
			break;
		}
		
		linkConnectionsWithHallway(connect[0]->connections, connect[1]->connections);
	}
	
	assert(allRoomConnectionsAreFree(rooms));
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