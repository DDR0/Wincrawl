#include <algorithm>
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

#include "seq.hpp"
#include "color.hpp"
#include "things.hpp"
#include "places.hpp"
#include "view.hpp"
#include "triggers.hpp"
#include "main_loop.hpp"

#define dbg std::raise(SIGINT)



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
	

	View view{ 23, 23, plane0.getStartingTile() };
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
	triggers.add("[1;3C", [&] { view.turn(+1); }); //cw
	triggers.add("[1;3D", [&] { view.turn(-1); }); //ccw
	
	//Windows arrow key sequences. (These are not valid utf8.)
	triggers.add("\xE0H", [&] { view.move(0); }); //up
	triggers.add("\xE0P", [&] { view.move(2); }); //down
	triggers.add("\xE0M", [&] { view.move(1); }); //left
	triggers.add("\xE0K", [&] { view.move(3); }); //right
	//triggers.add("???", [&] { view.turn(+1); }); //cw
	//triggers.add("???", [&] { view.turn(-1); }); //ccw

	
	triggers.add("q", [&]() { stopMainLoop = true; });
	triggers.add("", [&]() { stopMainLoop = true; }); //windows, ctrl-c
	
	
	cout << "Arrow keys to move, alt left/right to turn, q to quit.\n";
	runMainLoop(view, triggers);
	
	
	cout << "Good-bye.\n";
	return 0;
}