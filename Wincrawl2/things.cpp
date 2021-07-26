#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <vector>

#include "things.hpp"
#include "seq.hpp"


template<typename EventType>
void Event::Base::dispatch(Entity* entity, EventType* event)
	requires std::is_base_of<Event::Base, EventType>::value
{
	return nullptr;
}



Component::Base::Base(Entity* entityBeingEnhanced) : entity(entityBeingEnhanced) {}

void Component::Base::handleEvent(Event::Base* event) { }

Component::Health::Health(Entity* e, int startingHP): Base(e), hp(startingHP) {}

void Component::Health::handleEvent(Event::TakeDamage* evt)
{
	hp -= evt->amount;
	std::cerr << "Damage taken: " << evt->amount << ", hp remaining: " << hp << "\n";
}

void Component::Health::handleEvent(Event::DoAttack* attackDamage)
{
	//Fist!
	attackDamage->amount = 10;
	attackDamage->type.physical = true;
	attackDamage->type.blunt = true;
}

Component::Render::Render(Entity* e, const char* glyph_, Color color)
	: Base(e), glyph(glyph_), fgColor(color)
{

}
void Component::Render::handleEvent(Event::GetRendered* look)
{
	look->glyph = "@";
	look->fgColor = 0xDDA24E;
}
