#define dbg std::raise(SIGINT)

#include <iostream>
#include <random>
#include <unordered_map>
#include <memory>

#include "color.hpp"
#include "debug.hpp" 
#include "ecs.hpp"
#include "ecs.hpp"
#include "hashMapShimsForVisualStudio.hpp"
#include "io.hpp"
#include "main_loop.hpp"
#include "places.hpp"
#include "screen.hpp"
#include "seq.hpp"
#include "triggers.hpp"
#include "view.hpp"



Debug cerr{};

int main() {

	if (not setUpConsole()) {
		cerr << "Error: Could not set console to UTF8 mode.\n";
		return 255;
	};

	cerr << "⌛ Generating...\n";

	Tile tile0{};
	cerr << "tiles: " << tile0 << "\n";

	Tile tile1{};
	tile0.link(&tile1, 1, 3);
	cerr << "tiles: " << tile0 << " " << tile1 << "\n";

	Tile tile2{};
	tile0.insert(&tile2, 1, 2);
	cerr << "tiles: " << tile0 << " " << tile1 << " " << tile2 << "\n";
	
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
		DealDamage dam{ 10 };
		auto attack{ TakeDamage(avatar->dispatch(DealDamage{})) };
		cerr << "Attack amount: " << attack.amount << "\n";
		plane0.getStartingTile()->occupants.push_back(avatar);
	}
	
	{
		//Drop in a few enemies as well.
		using namespace Component;
		using namespace Event;
		for (int i=0; i < 3; i++) {
			Entity* enemy{ plane0.summon() };
			enemy->add<Existance>("g", 0xDDA24EFF);
			enemy->add<Fragility>(4);
			auto attack{ TakeDamage(enemy->dispatch(DealDamage{})) };
			cerr << "Attack amount: " << attack.amount << "\n";
			plane0.getStartingTile()->occupants.push_back(enemy);
		}
	}
	

	View view{ 23, 23, plane0.getStartingTile() };
	

	Color aColor = Color(Color::RGB(0xe6, 0x55, 0x51));
	cerr << aColor << "\n";
	auto bColor = Color(0x6d83cfff);
	cerr << bColor << "\n";
	auto cColor = Color(0x6d83cfff);
	cerr << cColor << " = " << (bColor == cColor) << "\n";
	
	
	//Let's create some screens to interact with the world through.
	std::shared_ptr<Screen> currentScreen = nullptr;
	enum class Screens { title, main, death, credits, debug };
	auto switchScreen = [&](std::shared_ptr<Screen> nextScreen) {
		currentScreen = nextScreen;
		currentScreen->setSize(80, 25);
	};
	const std::unordered_map<const Screens, const std::shared_ptr<Screen>, LiteralHash> screens {
		{ Screens::title, std::make_shared<TitleScreen>(
			Triggers{{
				{ "n", [&]{ switchScreen(screens.at(Screens::main)); } }, //new game
				{ "q", []{ stopMainLoop = true; } },
				{ "", []{ stopMainLoop = true; } }, //windows, ctrl-c
			}}
		) },
		
		{ Screens::main, std::make_shared<MainScreen>(
			&view,
			Triggers{{
				//Linux arrow key sequences.
				{ "[A", [&]{ view.move(0); } }, //up
				{ "[B", [&]{ view.move(2); } }, //down
				{ "[C", [&]{ view.move(1); } }, //left
				{ "[D", [&]{ view.move(3); } }, //right
				{ "[1;3C", [&]{ view.turn(+1); } }, //cw
				{ "[1;3D", [&]{ view.turn(-1); } }, //ccw
				
				//Windows arrow key sequences. (These are not valid utf8.)
				{ "\xE0H", [&]{ view.move(0); } }, //up
				{ "\xE0P", [&]{ view.move(2); } }, //down
				{ "\xE0M", [&]{ view.move(1); } }, //left
				{ "\xE0K", [&]{ view.move(3); } }, //right   \xE0 = à?
				{ "\x01\0x155", [&]{ view.turn(+1); } }, //cw - note, this sequence actually starts with a 0, but that doesn't work so well with string processing so we just add 1 to it.
				{ "\x01\0x157", [&]{ view.turn(-1); } }, //ccw
				
				//Other key sequences.
				{ "q", []{ stopMainLoop = true; } }, 
				{ "", []{ stopMainLoop = true; } }, //windows, ctrl-c
			}}
		) },
		
		{ Screens::death, std::make_shared<DeathScreen>(
			Triggers{{
				{ "n", [&]{ currentScreen = screens.at(Screens::main); } }, //new game
				{ "q", []{ stopMainLoop = true; } },
				{ "", []{ stopMainLoop = true; } }, //windows, ctrl-c
			}}
		) },
		
		{ Screens::credits, std::make_shared<CreditsScreen>(
			Triggers{{
				{ "r", [&]{ currentScreen = screens.at(Screens::main); } }, //return to game
				{ "q", []{ stopMainLoop = true; } },
				{ "", []{ stopMainLoop = true; } }, //windows, ctrl-c
			}}
		) },
		
		{ Screens::debug, std::make_shared<DebugScreen>(
			Triggers{{
				{ "r", [&]{ currentScreen = screens.at(Screens::main); } }, //return to game
				{ "q", []{ stopMainLoop = true; } },
				{ "", []{ stopMainLoop = true; } }, //windows, ctrl-c
			}}
		) },
	};

	switchScreen(screens.at(Screens::main));
	//currentScreen->render();
	//return 0;
	
	
	
	
	cerr << "Arrow keys to move, alt left/right to turn, q to quit.\n";
	runMainLoop(currentScreen);
	
	if (not tearDownConsole()) {
		cerr << "Error: Could not reset console.\n";
	}
		
	cerr << "Good-bye.\n";
	return 0;
}