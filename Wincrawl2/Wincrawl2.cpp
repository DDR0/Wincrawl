#define dbg std::raise(SIGINT)

#include <iostream>
#include <random>

#include "color.hpp"
#include "ecs.hpp"
#include "ecs.hpp"
#include "io.hpp"
#include "main_loop.hpp"
#include "places.hpp"
#include "screen.hpp"
#include "seq.hpp"
#include "triggers.hpp"
#include "view.hpp"



int main() {
	using namespace std;

	if (not setUpConsole()) {
		cerr << "Error: Could not set console to UTF8 mode.\n";
		return 255;
	};

	cout << "⌛ Generating...\n";

	Tile tile0{};
	cout << "tiles: " << tile0 << "\n";

	Tile tile1{};
	tile0.link(&tile1, 1, 3);
	cout << "tiles: " << tile0 << " " << tile1 << "\n";

	Tile tile2{};
	tile0.insert(&tile2, 1, 2);
	cout << "tiles: " << tile0 << " " << tile1 << " " << tile2 << "\n";
	
	std::minstd_rand rng { 6 }; //Note: Can call .seed(x) if needed. Does not return same on all platforms.
	(void) rng(); //Advance one step, initial value seems to be the seed otherwise. Cast to void to avoid VS unused value warning.
	Plane plane0{ rng, 10 };
	
	{
		//Drop the player into the world.
		using namespace Component;
		using namespace Event;
		Entity* avatar{ plane0.summon() };
		avatar->add<Existance>("@", 0xDDA24EFF);
		avatar->add<Fragility>(10);
		auto attack = TakeDamage(avatar->dispatch(DealDamage{}));
		std::cerr << "Attack amount: " << attack.amount << "\n";
		plane0.getStartingTile()->occupants.push_back(avatar);
	}
	

	View view{ 23, 23, plane0.getStartingTile() };
	view.render(cout);
	
	auto aColor = Color(Color::RGB(0xe6, 0x55, 0x51));
	cout << aColor << "\n";
	auto bColor = Color(0x6d83cfff);
	cout << bColor << "\n";
	auto cColor = Color(0x6d83cfff);
	cout << cColor << " = " << (bColor == cColor) << "\n";
	

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
	
	auto screen = TitleScreen();
	
	
	cout << "Arrow keys to move, alt left/right to turn, q to quit.\n";
	runMainLoop(triggers);
	
	if (not tearDownConsole()) {
		cerr << "Error: Could not reset console.\n";
	}
		
	cout << "Good-bye.\n";
	return 0;
}