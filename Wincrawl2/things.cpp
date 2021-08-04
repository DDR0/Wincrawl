#include <iostream>

#include "things.hpp"


void Component::Health::handleEvent(Event::TakeDamage* evt)
{
	std::cerr << "Ouch!\n";
	hp -= evt->amount;
	std::cerr << "Damage taken: " << evt->amount << ", hp remaining: " << hp << "\n";
}

void Component::Health::handleEvent(Event::DealDamage* attackDamage)
{
	std::cerr << "Smack!\n";

	//Fist!
	attackDamage->amount = 10;
	attackDamage->type.physical = true;
	attackDamage->type.blunt = true;
}


void Component::Render::handleEvent(Event::GetRendered* look)
{
	std::cerr << "It me!\n";

	look->glyph = glyph;
	look->fgColor = fgColor;
}