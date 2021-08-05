#include <iostream>

#include "things.hpp"


void Component::Health::handleEvent(Event::TakeDamage* evt)
{
	hp -= evt->amount;
	std::cerr << "Damage taken: " << evt->amount << ", hp remaining: " << hp << "\n";
}

void Component::Health::handleEvent(Event::DealDamage* attackDamage)
{
	//Fist!
	attackDamage->amount = 10;
	attackDamage->type.physical = true;
	attackDamage->type.blunt = true;
}


void Component::Render::handleEvent(Event::GetRendered* look)
{
	look->glyph = glyph;
	look->fgColor = fgColor;
}