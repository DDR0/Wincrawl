#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <vector>

#include "things.hpp"
#include "seq.hpp"


Component::Base::Base(Entity* entityBeingEnhanced) : entity(entityBeingEnhanced) {}

//void Component::Base::handleEvent(Event::Base* event) {
//	std::cerr << "Por nada.\n";
//}

Component::Health::Health(Entity* e, int startingHP): Base(e), hp(startingHP) {}

void Component::Health::handleEvent(Event::TakeDamage* evt)
{
	std::cerr << "Ouch!\n";
	hp -= evt->amount;
	std::cerr << "Damage taken: " << evt->amount << ", hp remaining: " << hp << "\n";
}

void Component::Health::handleEvent(Event::DoAttack* attackDamage)
{
	std::cerr << "Smack!\n";

	//Fist!
	attackDamage->amount = 10;
	attackDamage->type.physical = true;
	attackDamage->type.blunt = true;
}

Component::Render::Render(Entity* e, const char* glyph_, Color color)
	: Base(e), glyph(glyph_), fgColor(color) {}

void Component::Render::handleEvent(Event::GetRendered* look)
{
	std::cerr << "It me!\n";

	look->glyph = glyph;
	look->fgColor = fgColor;
}