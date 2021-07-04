//Classes related to places, the tiles of the map itself.
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <vector>

#include "places.hpp"
#include "seq.hpp"

//Darn it, C++. Remove element from vector.
//https://stackoverflow.com/questions/3385229/c-erase-vector-element-by-value-rather-than-by-position
template<typename T>
void filterOut(std::vector<T> vec, T elem) {
	vec.erase(std::remove(vec.begin(), vec.end(), elem), vec.end());
}


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

auto operator<<(std::ostream& os, Tile const& tile) -> std::ostream& {
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
	
	#ifndef NDEBUG
	//Don't allow resetting tile links here? Use insert for that. (Prevent one-way links. Jumps in perspective are not desired, you can always go back.)
	//This is more than just an assert because knowing **how** we messed up the linking is very helpful for development.
	const bool badLink {
		this->links[indexOut].tile() or
		other->links[indexIn].tile()
	};
	if (badLink) {
		std::cerr << "\nTile Link Error\n";
		
		std::cerr << "Tile " << this->getIDStr() << " indexOut " << (int)indexOut;
		if (this->links[indexOut].tile()) {
			std::cerr << " already points to tile "
				<< this->links[indexOut].tile()->getIDStr() << ".\n";
		} else {
			std::cerr << " clear.\n";
		}
		std::cerr << (this->listLinks(indexOut)) << "\n";
		
		std::cerr << "Tile " << other->getIDStr() << " indexOut " << (int)indexIn;
		if (other->links[indexIn].tile()) {
			std::cerr << " already points to tile "
				<< other->links[indexIn].tile()->getIDStr() << ".\n";
		} else {
			std::cerr << "clear.\n";
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



Plane::Room Plane::genSquareRoom(
	const uint8_t roomX, const uint8_t roomY, 
	const bool wrapX, const bool wrapY,
	const Color fg, const Color bg
) {
	Tile* room[roomX][roomY] = {};

	for (uint8_t x = 0; x < roomX; x++) {
		for (uint8_t y = 0; y < roomY; y++) {
			Tile* tile{ tiles.emplace_back(new Tile()) };
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
	
	std::vector<RoomConnection> connections {};
	if (!wrapX) {
		connections.emplace_back(room[roomX-1][roomY/2], 1);
		connections.emplace_back(room[      0][roomY/2], 3);
	}
	if (!wrapY) {
		connections.emplace_back(room[roomX/2][      0], 0);
		connections.emplace_back(room[roomX/2][roomY-1], 2);
	}
	
	std::cout << "Genned room.\n";
	for (auto c : connections) {
		std::cout << "Tile " << c.tile->getIDStr() << ": " << c.tile->listLinks(c.dir) << "\n";
	}
	
	return Room{ room[roomX/2][roomY/2], connections };
}

Plane::Plane(uint16_t numRooms)
	: id(TotalPlanesCreated++)
{
	tiles.reserve(512);
	rooms.emplace_back(genSquareRoom(10, 5, true, false, Color{217, 62, 60}));
	rooms.emplace_back(genSquareRoom(10, 5, false, false, Color{146, 62, 60}));
	
	RoomConnection doorA1{ rooms.front().connections.front() };
	RoomConnection doorA2{ rooms.front().connections.back() };
	RoomConnection doorB1{ rooms.back().connections.front() };
	RoomConnection doorB2{ rooms.back().connections.back() };
	
	std::cerr << "Linking " << doorA1.tile->listLinks(doorA1.dir) << " to " << doorB2.tile->listLinks(doorB2.dir) << ".\n";
	doorA1.tile->link(doorB2.tile, doorA1.dir, doorB2.dir);
	
	std::cerr << "Linking " << doorA2.tile->listLinks(doorA2.dir) << " to " << doorB1.tile->listLinks(doorB1.dir) << ".\n";
	doorA2.tile->link(doorB1.tile, doorA2.dir, doorB1.dir);
	
	Tile* hall1{ tiles.emplace_back(new Tile()) };
	std::cerr << "Inserting from " << doorA1.tile->listLinks(doorA1.dir) << " in " << (int)doorA1.dir << ".\n";
	doorA1.tile->insert(hall1, doorA1.dir);
	std::cerr << "Inserted " << hall1->getIDStr() << " " << hall1->listLinks() << ".\n";
	
	Tile* hall2{ tiles.emplace_back(new Tile()) };
	std::cerr << "Inserting from " << doorA2.tile->listLinks(doorA1.dir) << " in " << (int)doorA2.dir << ".\n";
	doorA2.tile->insert(hall2, doorA2.dir);
	std::cerr << "Inserted " << hall2->listLinks() << ".\n";
	
	Entity* avatar{ entities.emplace_back(new Entity()) };
	avatar->glyph = "@";
	avatar->fgColor = 0xDDA24E;
	rooms.at(0).seed->occupants.push_back(avatar);
}

Plane::~Plane() {
	for (auto tile : tiles) { delete tile; }
	for (auto entity : entities) { delete entity; }
}

auto operator<<(std::ostream& os, Plane const& plane) -> std::ostream& {
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