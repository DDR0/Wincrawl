#include <random>

#include "places.hpp"
#include "screen.hpp"
#include "triggers.hpp"
#include "view.hpp"



int main() {
	std::minstd_rand rng { 6 }; //Note: Can call .seed(x) if needed. Does not return same on all platforms.
	Plane plane0{ rng, 10 };
	View view{ 10, 10, plane0.getStartingTile() };
	MainScreen screen{ view };
}