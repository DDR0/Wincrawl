#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include "hsluv.h"
#include "io.hpp"

#ifdef _MSC_VER
//unsigned int _lzcnt_u32 (unsigned int a)
#include "immintrin.h"
#define COUNT_LEADING_ZEROS _lzcnt_u32
#else
//int __builtin_clz (unsigned int x)
#define COUNT_LEADING_ZEROS __builtin_clz
#endif

#define dbg std::raise(SIGINT)

struct seq {
	inline static const char* clear { "c" };
	inline static const char* reset { "[0m" };
	inline static const char* bold  { "[1m" };
};

class Color {
	//Encapsulate https://www.hsluv.org/ in a convenient wrapper.
	uint8_t channels[3]{};
public:
	struct RGB { uint8_t r; uint8_t g; uint8_t b; };
	struct HSL { double h; double s; double l; };
	
	Color() : channels{0, 0, 0} {}
	
	Color(uint32_t rgb) : channels { 
		static_cast<uint8_t>(rgb>>16),
		static_cast<uint8_t>(rgb>>8&0xFF),
		static_cast<uint8_t>(rgb&0xFF)
	} { assert(rgb <= 0xffffff); }
	
	Color(RGB other) : channels{ other.r, other.g, other.b } {}
	
	Color(double h, double s, double l) {
		double channelsIn[3];
		hsluv2rgb(h, s, l,
			&channelsIn[0], &channelsIn[1], &channelsIn[2]);
		
		for (int i = 0; i < 3; ++i)
			channels[i] = int(std::round(channelsIn[i]*255));
	}
	
	Color(HSL other) {
		double channelsIn[3];
		hsluv2rgb(other.h, other.s, other.l,
			&channelsIn[0], &channelsIn[1], &channelsIn[2]);
		
		for (int i = 0; i < 3; ++i)
			channels[i] = int(std::round(channelsIn[i]*255));
	}
	Color(Color& other) : channels{ other[0], other[1], other[2] } {}

	void rgb(uint8_t r, uint8_t g, uint8_t b) {
		channels[0] = r;
		channels[1] = g;
		channels[2] = b;
	}
	
	void hsl(double h, double s, double l) {
		double channelsIn[3];
		hsluv2rgb(h, s, l,
			&channelsIn[0], &channelsIn[1], &channelsIn[2]);
		
		for (int i = 0; i < 3; ++i)
			channels[i] = int(std::round(channelsIn[i]*255));
	}

	RGB rgb() const {
		return RGB(channels[0], channels[1], channels[2]);
	}

	HSL hsl() const {
		HSL newcolour;
		rgb2hsluv(
			channels[0]/255., channels[1]/255., channels[2]/255., 
			&newcolour.h, &newcolour.s, &newcolour.l
		);
		return newcolour;
	}

	Color& operator=(const Color& other) {
		for (int i = 0; i < 3; i++)
			channels[i] = other[i];
		return *this;
	}

	uint8_t operator[](size_t i) {
		assert(i < 3);
		return channels[i];
	}

	uint8_t operator[](size_t i) const {
		assert(i < 3);
		return channels[i];
	}

	friend auto operator<<(std::ostream& os, Color const& color) -> std::ostream& {
		const HSL hsl = color.hsl();
		return os
			<< "HSLuv(" //display values
			<< std::to_string(int(std::round(hsl.h))) << ","
			<< std::to_string(int(std::round(hsl.s))) << ","
			<< std::to_string(int(std::round(hsl.l))) << ")"

			<< "[38;2;" //display color
			<< std::to_string(color[0]) << ";"
			<< std::to_string(color[1]) << ";"
			<< std::to_string(color[2]) << "m﹅[0m";
	}
	
	const std::string fg() const {
		return "[38;2;"
			+ std::to_string(this->channels[0]) + ";"
			+ std::to_string(this->channels[1]) + ";"
			+ std::to_string(this->channels[2]) + "m";
	}
	
	const std::string bg() const {
		return "[48;2;"
			+ std::to_string(this->channels[0]) + ";"
			+ std::to_string(this->channels[1]) + ";"
			+ std::to_string(this->channels[2]) + "m";
	}
};


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

	Tile() : id(TotalTilesCreated++) {}
	
	std::string listLinks(int8_t hightlightIndex = -1) {
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

	void insert(Tile* other, int8_t indexOut, int8_t indexIn = -1) {
		//Put a tile between two connected tiles. (Neither of the tiles otherwise move.)

		if (indexIn == -1) {
			indexIn = oppositeEdge[indexOut];
		}

		assert(other);
		assert(indexOut != indexIn); //Indices in are for the internal tile, since we know the outter tile indices this time.
		assert(indexOut >= 0 && indexOut < 6); //Direction index out of this tile must be specified.
		assert(indexIn >= 0 && indexIn < 6);
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

	Link* getNextTile(int comingFrom, int pointingIn) {
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

	Link* getNextTile(int directionIndex) {
		assert(0 <= directionIndex && directionIndex < 6);
		return &(links[directionIndex]);
	}

	std::string getIDStr() {
		constexpr int idDigitLength = 4;
		auto buf = std::make_unique<char[]>(idDigitLength);
		std::snprintf(buf.get(), idDigitLength, "%0*lu", idDigitLength - 1, id);
		return std::string(buf.get());
	}
};


class Plane {
	//A plane is a collection of tiles, which are formed into rooms.
	
	inline static uint_fast16_t TotalPlanesCreated{ 0 };
	const uint_fast16_t id{ 0 };

	std::vector<Tile*> tiles; //List of all tiles we created.
	
	struct RoomConnection {
		Tile* tile;
		int8_t dir;
	};
	
	struct Room {
		Tile* seed;
		std::vector<RoomConnection> connections;
	};
	std::vector<Room> rooms {};
	
	Room genSquareRoom(
		const uint8_t roomX, const uint8_t roomY, 
		const bool wrapX = false, const bool wrapY = false,
		const Color fg = Color{0,0,100},
		const Color bg = Color{0,0,0}
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
		
		return Room{room[roomX/2][roomY/2], connections};
	}
	

public:
	Plane(uint16_t numRooms)
		: id(TotalPlanesCreated++)
	{
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
		std::cerr << "Inserting from " << " (" << doorA1.tile->listLinks(doorA1.dir) << ") in " << (int)doorA1.dir << ".\n";
		doorA1.tile->insert(hall1, doorA1.dir);
		std::cerr << "Inserted " << hall1->getIDStr() << " " << hall1->listLinks() << ".\n";
		
		Tile* hall2{ tiles.emplace_back(new Tile()) };
		std::cerr << "Inserting from " << " (" << doorA2.tile->listLinks(doorA1.dir) << ") in " << (int)doorA2.dir << ".\n";
		doorA2.tile->insert(hall2, doorA2.dir);
		std::cerr << "Inserted " << hall2->listLinks() << ".\n";
		
		//A statue of a person, let's say.
		//room[2][3]->glyph = "@";
		//room[2][3]->isOpaque = true;
	}

	~Plane() {
		for (auto tile : tiles) {
			delete tile;
		}
	}

	friend auto operator<<(std::ostream& os, Plane const& plane) -> std::ostream& {
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

	Tile* getStartingTile() {
		return rooms.at(0).seed;
	}
};



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
		RayWalker(std::vector<std::vector<Tile*>>* field_) : field(field_) {}

		void reset(Tile* startingTile, int rot, int x, int y) {
			loc = startingTile;
			dir = rot;

			lastX = x;
			lastY = y;
			lastDirectionIndex = 0;

			//std::cout << "Reset raywalker.\n";
		}

		bool moveTo(int x, int y) { //Must be within the bounds of field.
			const int currentDeltaX = x - lastX;
			const int currentDeltaY = y - lastY;

			//std::cout << "moved to " << x << "×" << y << " (" << currentDeltaX << "×" << currentDeltaY << ")\n";
			int directionIndex;
			if (currentDeltaY == +1) directionIndex = 0; else
			if (currentDeltaX == +1) directionIndex = 1; else
			if (currentDeltaY == -1) directionIndex = 2; else
			if (currentDeltaX == -1) directionIndex = 3; else {
				return true; //No motion, stay where we are.
			}

			if (not(lastX || lastY)) {
				//Starting off, so relative movement to our tile.
				auto movement = loc->getNextTile(directionIndex);
				loc = movement->tile();
				lastDirectionIndex = dir = movement->dir();
				//std::cout << "moved! " << directionIndex << "\n";
			}
			else {
				//Enter the room in the relative direction from us.
				//std::cout << "moved: " << directionIndex << " (from " << lastDirectionIndex << " is " << (directionIndex-lastDirectionIndex) << ")\n";
				auto movement = loc->getNextTile(dir, directionIndex - lastDirectionIndex);
				loc = movement->tile();
				dir = movement->dir();
				lastDirectionIndex = directionIndex;
			}

			lastX = x;
			lastY = y;

			if (loc) {
				//std::cout << "setting " << x << "×" << y << " to #" << loc->getIDStr() << " (" << reinterpret_cast<const char*>(loc->glyph) << ")\n";
				(*field)[x][y] = loc;
				return !loc->isOpaque; //Continue tracing only if whatever we're looking at isn't solid.
			}
			else {
				//std::cout << "setting " << x << "×" << y << " to #" << emptyTile.getIDStr() << " (" << reinterpret_cast<const char*>(emptyTile.glyph) << ")\n";
				(*field)[x][y] = &emptyTile;
				return false;
			}
		}
	};

	uint8_t viewSize[2];
	std::vector<std::vector<Tile*>> grid;
	inline static Tile tempAvatar{};
	inline static Tile hiddenTile{};
	inline static Tile emptyTile{};

	RayWalker rayWalker{ &grid };

public:
	Tile* loc;
	int rot{ 0 };

	View(uint8_t width, uint8_t height, Tile* pointOfView)
		: loc(pointOfView)
	{
		viewSize[0] = width;
		viewSize[1] = height;

		grid.reserve(width);
		for (int x = 0; x < viewSize[0]; x++) {
			grid.emplace_back();
			grid[x].assign(height, nullptr);
		}

		tempAvatar.roomId = 0;
		tempAvatar.glyph = "@";
		hiddenTile.roomId = 1;
		hiddenTile.glyph = "░";
		emptyTile.roomId = 2;
		emptyTile.glyph = "▓";
	}

	void render(std::ostream& target) {
		assert(loc); //If no location is defined, fail.

		int viewloc[2] = { viewSize[0] / 2, viewSize[1] / 2 };

		//Tile* tilemap[viewSize[0]][viewSize[1]] = {};

		//target << reinterpret_cast<const char*>(loc->glyph) << "\n";
		std::cout << "starting\n";

		//First, all our tiles are hidden.
		for (int x = 0; x < viewSize[0]; x++) {
			for (int y = 0; y < viewSize[1]; y++) {
				grid[x][y] = &hiddenTile;
			}
		}

		for (int x = 0; x < viewSize[0]; x += viewSize[0] - 1) {
			for (int y = 0; y < viewSize[1]; y++) {
				this->raytrace(rayWalker, viewloc[0], viewloc[1], x, y);
			}
		}
		for (int x = 0; x < viewSize[0]; x++) {
			for (int y = 0; y < viewSize[1]; y += viewSize[1] - 1) {
				this->raytrace(rayWalker, viewloc[0], viewloc[1], x, y);
			}
		}
		//this->raytrace(rayWalker, viewloc[0], viewloc[1], viewSize[0]-1, 0);
		grid[viewloc[0]][viewloc[1]] = &tempAvatar;

		for (int y = 0; y < viewSize[1]; y++) {
			for (int x = 0; x < viewSize[0]; x++) {
				target
					<< grid[x][y]->fgColor.fg() << grid[x][y]->bgColor.bg()
					<< reinterpret_cast<const char*>(grid[x][y]->glyph)
					<< seq::reset;
				//target << grid[x][y]->getIDStr();
			}
			target << "\n";
		}
		target << "ending\n";
	}

	//Borrowed and modified from http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html
	void raytrace(RayWalker&, int x0, int y0, int x1, int y1)
	{
		//std::cout << "trace: " << x0 << "→" << x1 << ", " << y0 << "→" << y1 << "\n";
		int dx{ abs(x1 - x0) };
		int dy{ abs(y1 - y0) };
		int x{ x0 };
		int y{ y0 };
		int x_inc{ (x1 > x0) ? 1 : -1 };
		int y_inc{ (y1 > y0) ? 1 : -1 };
		int error = dx - dy;
		dx *= 2;
		dy *= 2;

		rayWalker.reset(loc, rot, x, y);

		for (int n = (dx + dy) / 2; n > 0; --n) {
			if (error > 0) {
				x += x_inc;
				error -= dy;
			}
			else {
				y += y_inc;
				error += dx;
			}

			//std::cout << "rayloc: " << x << "×" << y << ", ";
			if (!rayWalker.moveTo(x, y)) {
				break;//If the rayWalker has moved to an invalid location, stop tracing.
			};
		}
	}
	
	void moveCamera(int direction) {
		auto link {loc->getNextTile((direction + rot + 4) % 4) };
		if (!link->tile()) return;

		//To get the new view rotation, consider the following example
		//of travelling between two tiles in direction 1.
		//
		//  view.rot=1    view.rot=0
		//  ┌─────┐       ┌─────┐
		//  │  1  │ dir 1 │  0  │
		//  │0   2│ ────→ │3   1│
		//  │  3  │       │  2  │
		//  └─────┘       └─────┘
		//   tile1         tile2
		//
		//To calculate the new view rotation, we get our rotational
		//delta by subtracting the opposite of the destination arrival
		//direction (which we entered from) from the direction we left
		//from. In this case, we leave from 2 and arrive at 3, so the
		//opposite direction is 1. This is pointing, relatively
		//speaking, in the same direction we left. We will keep it
		//aligned by adding the delta between the two directions to
		//our rotation. Delta is 1-2=-1, so rotation is 1+-1=0.
		//
		//Thus, the following formula breaks down like so:
		//Inner-most brackets: Tile direction left in.
		//Up one level: Rotatinoal delta calculation.
		//Rest of formula: Add rotational delta to current rotation.
		std::cerr << "Moved from " << loc->listLinks()
			<< " facing " << (int)rot
			<< " in " << direction 
			<< ", over " << (int)link->dir()
			<< ".\n";
		
		
		rot = (rot + (
			Tile::oppositeEdge[link->dir()] - (direction + rot)
		) + 4) % 4;
		loc = link->tile();
		
		std::cerr << "Arrived on " << loc->listLinks()
			<< " facing " << (int)rot
			<< ".\n";
	}
	
	void move(int direction) {
		moveCamera(direction);
		//std::cout << seq::clear;
		render(std::cout);
	}
};



class Triggers {
	typedef bool isPartialCommand;
	
	struct Trigger {
		const char* seq;
		const std::function<void()> callback;
	};
	
	std::vector<Trigger> triggers{};
	
	
public:
	//If you're feeding a character stream buffer to run(...), isPartialCommand can be
	//used to decide to wait for more characters to be added to the buffer.
	const isPartialCommand run(const char* cmd) {
		const char* command = reinterpret_cast<const char*>(cmd); //Cast for str* functions - it's safe, and we only care about byte-level compatability in this case.
		
		int commandMatchesTriggers{ false };
		for (auto trigger : this->triggers) {
			const char* triggerName = reinterpret_cast<const char*>(trigger.seq);
			
			//Command should be fully equal to run the cb - the full match is needed to absorb all of an escape sequence which could be unique by the second character.
			if (!strcmp(triggerName, command)) {
				trigger.callback();
				return false;
			}
			
			//Otherwise, mark if our command was equal to part of a trigger. It could be completed in the future.
			commandMatchesTriggers |=
				!strncmp(triggerName, command, strlen(command));
		}
		return commandMatchesTriggers;
	}
	
	void add(const char* seq, const std::function<void()> callback) {
		this->triggers.emplace_back(seq, callback);
	}
};


bool stopMainLoop{ false };
void runMainLoop(View& view, Triggers& triggers) {
	using namespace std::chrono_literals;
	
	std::atomic<int> chr{ -1 };
	std::jthread inputListener(getInputCharAsync, ref(chr));

	const int maxInputBufferLength = 8;
	int inputBuffered{ 0 };
	char inputBuffer[maxInputBufferLength]{};

	std::cout << "> ";
	while (true) {
		auto nextFrame = std::chrono::steady_clock::now() + 16ms;
		int chr_ = chr.load();
		if (chr_ >= 0) {
			//chr_ is, presumably, a utf32 character. We use utf8 internally. Convert.
			auto codepoints{ 4 - COUNT_LEADING_ZEROS(chr_) / 8 };
			inputBuffered += codepoints;
			assert(inputBuffered + 1 < maxInputBufferLength); //Last char must be reserved for 0.
			for (; codepoints; codepoints--)
				inputBuffer[inputBuffered - codepoints] =
				(chr_ >> 8 * (codepoints - 1)) & 0xFF;

			if (!triggers.run(inputBuffer)) {
				inputBuffered = 0;
				for (int i = 0; i < maxInputBufferLength; i++)
					inputBuffer[i] = 0;
			}

			std::cout << std::hex << chr_ << " " << (char)chr_ << " (" << reinterpret_cast<const char*>(inputBuffer) << ")" << "\n> ";

			if (!chr_ || chr_ == 4 || stopMainLoop) { //Null, ctrl-d.
				chr = getInputCharAsync::stop;
				break;
			}
			else {
				chr = getInputCharAsync::next; //Next char.
			}
		}
		std::this_thread::sleep_until(nextFrame);
	}
}



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

	cout << "Done!\n";

	View view{ 20, 20, plane0.getStartingTile() };
	view.render(cout);
	
	
	auto aColor = Color(Color::RGB(0xe6, 0x55, 0x51));
	cout << aColor << "\n";
	auto bColor = Color(305, 91, 56);
	cout << bColor << "\n";
	auto cColor = Color(0x6d83cf);
	cout << cColor << "\n";
	
	
	Triggers triggers{};
	
	//Linux arrow key sequences.
	triggers.add("[A", [&]{ view.move(0); }); //up
	triggers.add("[B", [&]{ view.move(2); }); //down
	triggers.add("[C", [&]{ view.move(1); }); //left
	triggers.add("[D", [&]{ view.move(3); }); //right
	
	//Windows arrow key sequences. (These are not valid utf8.)
	triggers.add((const char*)"\xE0H", [&] { view.move(0); }); //up
	triggers.add((const char*)"\xE0P", [&] { view.move(2); }); //down
	triggers.add((const char*)"\xE0M", [&] { view.move(1); }); //left
	triggers.add((const char*)"\xE0K", [&] { view.move(3); }); //right
	
	triggers.add("q", [&]() { stopMainLoop = true; });
	triggers.add("", [&]() { stopMainLoop = true; }); //windows, ctrl-c
	
	
	runMainLoop(view, triggers);
	
	
	cout << "Good-bye.\n";
	return 0;
}