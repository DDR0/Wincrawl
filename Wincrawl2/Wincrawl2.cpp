#define dbg std::raise(SIGINT)

#include <iostream>
#include <random>

#include "io.hpp"
#include "seq.hpp"
#include "color.hpp"
#include "things.hpp"
#include "places.hpp"
#include "view.hpp"
#include "triggers.hpp"
#include "main_loop.hpp"

class A {
public:
	int valA() { return 1; }
	virtual int valB() { return 2; };
};

class B : public A {
public:
	int valA() { return 3; }
	int valB() { return 4; }
};

class E : public A {
public:
	int valA() { return 5; }
	int valB() { return 6; }
};

class C {
public:
	virtual int getVal(A* cont) {
		return cont->valB();
	}
};

class D: public C {
public:
	int getVal(A* cont) {
		return cont->valB();
	}
	int getVal(B* cont) {
		return cont->valA();
	}
};


int main() {
	//A* a{ new B() };
	//B* b{ new B() };
	//E* e{ new E() };
	//std::cout << "Test: " << a->valA() << " " << a->valB() << "\n"; // 1 4
	//C* c{ new C() };
	//D* d{ new D() };
	//B* f{ new B() };
	//
	//std::cout << "Test: " << c->getVal(a) << " " << d->getVal(a) << "\n"; // 4 4
	//std::cout << "Test: " << c->getVal(b) << " " << d->getVal(b) << "\n"; // 4 3
	//
	//return 0;

	using namespace std;

	if (not setUpConsole()) {
		cout << "Error: Could not set console to UTF8 mode.\n";
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
	
	std::minstd_rand rng { 5 }; //Note: Can call .seed(x) if needed. Does not return same on all platforms.
	(void) rng(); //Advance one step, initial value seems to be the seed otherwise. Cast to void to avoid VS unused value warning.
	Plane plane0{ rng, 4 };

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
	runMainLoop(triggers);
	
	
	cout << "Good-bye.\n";
	return 0;
}