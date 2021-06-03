#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include "io.cpp"

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
	static constexpr uint8_t rotateCW[6]{ 1, 2, 3, 0, 1, 3 }; //Rotate around the Z axis, ie, top-down.
	static constexpr uint8_t rotateCCW[6]{ 3, 0, 1, 2, 3, 1 }; //Going around a corner from the top will land you "facing" east or west, although it could just as easily be north and south as your rotation isn't tracked.

public:
	//Since we are in a non-euclidean space here, N/E/S/W and Up/Down directions don't really make any sense.
	//However, if it helps, you can think of the links array as being such where N=0.
	Link links[6]{};
	uint8_t roomId{ 0 }; //0=uninitialized, 1=hidden, 2=empty, 9=hallway, 10≥rooms
	const char8_t* glyph { u8" " }; //String, 4 bytes + null terminator for utf8 astral plane characters.
	bool isOpaque{ false };

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
		}
	}
	
	Link* getNextTile(int directionIndex) {
		assert(0 <= directionIndex && directionIndex < 6);
		return &(links[directionIndex]);
	}
	
	std::string getIDStr() {
		auto buf = std::make_unique<char[]>( 4 );
		std::sprintf(buf.get(), "%0*lu", 3, id );
		return std::string(buf.get());
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
				tile->roomId = 10;
				tile->glyph = (x + y) % 2 ? u8"," : u8".";

				tiles.push_back(tile);
				room[x][y] = tile;
			}
		}

		for (uint8_t x = 0; x < roomX - 0; x++) { //-0 to loop, -1 to not
			for (uint8_t y = 0; y < roomY; y++) {
				room[x][y]->link(room[(x + 1) % roomX][y], 1);
			}
		}

		for (uint8_t x = 0; x < roomX; x++) {
			for (uint8_t y = 0; y < roomY - 1; y++) {
				room[x][y]->link(room[x][(y + 1) % roomY], 2);
			}
		}

		for (uint8_t y = 0; y < roomY; y++) {
			for (uint8_t x = 0; x < roomX; x++) {
				std::cerr << room[x][y]->getIDStr() << " ";
			}
			std::cerr << "\n";
		}

		//A statue of a person, let's say.
		roomSeeds.push_back(room[2][3]);
		room[2][3]->glyph = u8"@";
		room[2][3]->isOpaque = true;
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

	class RayWalker {
		//Since our geometry has no external location or orientation, we must "walk" it to
		//find out what we've got. Our RayWalker translates absolute movement of the raytrace
		//function into relative movement through the world.
		//It notes down what it's found in the field.
		
		Tile* loc; //location
		int dir {0}; //direction
		
		int lastX {0};
		int lastY {0};
		int lastDirectionIndex {0};
		
		typedef std::vector<std::vector<Tile*>>* Field;
		Field field {};
		
	public:
		RayWalker(std::vector<std::vector<Tile*>>* field_) : field(field_) {}
		
		void reset(Tile* startingTile, int x, int y) {
			loc = startingTile;
			dir = 0;
			
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
			
			if(not(lastX||lastY)) {
				//Starting off, so relative movement to our tile.
				auto movement = loc->getNextTile(directionIndex);
				loc = movement->tile();
				lastDirectionIndex = dir = movement->dir();
				//std::cout << "moved! " << directionIndex << "\n";
			} else {
				//Enter the room in the relative direction from us.
				//std::cout << "moved: " << directionIndex << " (from " << lastDirectionIndex << " is " << (directionIndex-lastDirectionIndex) << ")\n";
				auto movement = loc->getNextTile(dir, directionIndex-lastDirectionIndex);
				loc = movement->tile();
				dir = movement->dir();
				lastDirectionIndex = directionIndex;
			}
			
			lastX = x;
			lastY = y;
			
			if(loc) {
				//std::cout << "setting " << x << "×" << y << " to #" << loc->getIDStr() << " (" << reinterpret_cast<const char*>(loc->glyph) << ")\n";
				(*field)[x][y] = loc;
				return !loc->isOpaque; //Continue tracing only if whatever we're looking at isn't solid.
			} else {
				//std::cout << "setting " << x << "×" << y << " to #" << emptyTile.getIDStr() << " (" << reinterpret_cast<const char*>(emptyTile.glyph) << ")\n";
				(*field)[x][y] = &emptyTile;
				return false;
			}
		}
	};
	
	uint8_t viewSize[2];
	std::vector<std::vector<Tile*>> grid;
	inline static Tile hiddenTile {};
	inline static Tile emptyTile {};
	
	RayWalker rayWalker{ &grid };

public:
	Tile* loc;
	
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
		
		hiddenTile.roomId = 1;
		hiddenTile.glyph = u8" ";
		emptyTile.roomId = 2;
		emptyTile.glyph = u8"▓";
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
		
		for (int x = 0; x < viewSize[0]; x+=viewSize[0]-1) {
			for (int y = 0; y < viewSize[1]; y++) {
				this->raytrace(rayWalker, viewloc[0], viewloc[1], x, y);
			}
		}
		for (int x = 0; x < viewSize[0]; x++) {
			for (int y = 0; y < viewSize[1]; y+=viewSize[1]-1) {
				this->raytrace(rayWalker, viewloc[0], viewloc[1], x, y);
			}
		}
		//this->raytrace(rayWalker, viewloc[0], viewloc[1], viewSize[0]-1, 0);
		grid[viewloc[0]][viewloc[1]] = loc;
		
		for (int y = 0; y < viewSize[1]; y++) {
			for (int x = 0; x < viewSize[0]; x++) {
				target << reinterpret_cast<const char*>(grid[x][y]->glyph);
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
		int dx { abs(x1 - x0) };
		int dy { abs(y1 - y0) };
		int x { x0 };
		int y { y0 };
		int x_inc { (x1 > x0) ? 1 : -1 };
		int y_inc { (y1 > y0) ? 1 : -1 };
		int error = dx - dy;
		dx *= 2;
		dy *= 2;
		
		rayWalker.reset(loc, x, y);

		for (int n = (dx + dy)/2; n > 0; --n) {
			if (error > 0) {
				x += x_inc;
				error -= dy;
			} else {
				y += y_inc;
				error += dx;
			}
			
			//std::cout << "rayloc: " << x << "×" << y << ", ";
			if (!rayWalker.moveTo(x, y)) {
				break;//If the rayWalker has moved to an invalid location, stop tracing.
			};
		}
	}
};


int main() {
	using namespace std;
	using namespace chrono_literals;
	
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

	string command {};
	int chr { -1 };
	std::jthread inputListener(getInputCharAsync, &chr);
	cout << "> ";
	while (true) {
		auto nextFrame = std::chrono::steady_clock::now() + 16ms;
		if (chr >= 0) {
			cout << chr << "\n> ";
			
			if (!chr || chr==4 || chr==27) break; //Null, ctrl-d, esc.
			
			chr = -1;
		}
		std::this_thread::sleep_until(nextFrame);
	}
	chr = -2;
	cout << "End.\n";
	return 0;
}