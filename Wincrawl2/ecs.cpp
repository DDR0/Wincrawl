#include <cassert>
#include <iostream>

#include "ecs.hpp"


void Component::Fragility::handleEvent(Event::TakeDamage* evt)
{
	hp -= evt->amount;
	std::cerr << "Damage taken: " << evt->amount << ", hp remaining: " << hp << "\n";
}

void Component::Fragility::handleEvent(Event::DealDamage* attackDamage)
{
	//Fist!
	attackDamage->amount = 10;
	attackDamage->type.physical = true;
	attackDamage->type.blunt = true;
}


void Component::Existance::handleEvent(Event::GetRendered* look)
{
	look->glyph = glyph;
	look->fgColor = fgColor;
}

void Component::Existance::handleEvent(Event::AddSubentity* evt) {
	if(!evt->entity) return;
	Entity* e = evt->entity;
	subentities.insert(e);
	evt->success = true;
};

void Component::Existance::handleEvent(Event::RemoveSubentity* evt) {
	if(!evt->entity) return;
	Entity* e = evt->entity;
	subentities.erase(e);
	evt->success = true;
};

void Component::Existance::handleEvent(Event::MoveTo* evt) {
	auto rem = superentity->dispatch(Event::RemoveSubentity{{.entity=entity, .force=evt->force}});
	if (!rem.success) return;
	auto add = evt->entity->dispatch(Event::AddSubentity{{.entity=entity, .force=evt->force}});
	if (!add.success) {
		auto readd = superentity->dispatch(Event::AddSubentity{{.entity=entity, .force=true}});
		assert(("Could not add object to destination, could not return object to origin.", readd.success));
	} else {
		superentity = evt->entity;
		evt->success = add.success;
	}
	evt->success = add.success;
};