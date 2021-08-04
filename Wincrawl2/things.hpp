//Classes related to things, entities, on the map.
#pragma once

#include <type_traits>
#include <memory>
#include <set>
#include <vector>

#include "color.hpp"
#include "places.hpp"
#include "vector_tools.hpp" //Only needed by rem(Component::Base).

class Tile; //Not defined by places.hpp, as places.hpp requires this first.
class Entity;

namespace Event {
	//An event is something which happens. Eg., you stab something or get stabbed, you are drawn on screen, etc.

	struct Base {};

	struct Damage: Base {
		int amount{ 0 };
		struct type {
			bool physical{};
			bool blunt{};
			bool pierce{};
			bool slash{};
			bool mental_capacity{};
			bool fire{};
			bool ice{};
			bool bullet{};
			bool shockwave{};
		};
		type type{};
	};
	struct DealDamage: Damage {};
	struct TakeDamage: Damage {};

	struct GetRendered : public Base {
		const char* glyph{ nullptr };
		Color fgColor{ 0xFF0000 };
		Color bgColor{ 0xFF0000 }; //needs some concept of an alpha channel, so disused for now
	};
};


namespace Component {
	using namespace Component;
	
	enum class Priority { last, neutral, bonusModifier, baseModifier, first };

	class Base {
	public:
		virtual constexpr Priority priority() const { return Priority::neutral; };
		virtual const char* name() const { return "Base"; };

		Entity* entity{ nullptr };
		Base(Entity* e) : entity(e) {}
		
		//Ideally, we'd figure out a way to make orderByPriority() use this, but I can't get it to work.
		std::weak_ordering operator<=>(const Base& other) const {
			return priority() <=> other.priority();
		};
		
		struct orderByPriority {
			bool operator() (const auto& lhs, const auto& rhs) const {
				return lhs->priority() < rhs->priority();
			}
		};

		//Specializations are needed, otherwise base just gets called.
		virtual void handleEvent(Event::TakeDamage*) {};
		virtual void handleEvent(Event::DealDamage*) {};
		virtual void handleEvent(Event::GetRendered*) {};
	};

	class Health : public Base {
	public:
		const char* name() const override { return "Health"; };
		
		int hp{ 10 };

		Health(Entity* e, int startingHP): Base(e), hp(startingHP) {}

		void handleEvent(Event::TakeDamage*) override;
		void handleEvent(Event::DealDamage*) override;
	};

	class Render : public Base {
	public:
		const char* name() const override { return "Render"; };
		
		const char* glyph{ "￼" };
		Color fgColor{ 0xFF0000 };
		uint8_t type{ 0 };
		uint8_t zorder{ 0 };

		Render(Entity* e, const char* glyph_, Color color)
			: Base(e), glyph(glyph_), fgColor(color) {}

		void handleEvent(Event::GetRendered*) override;
	};
};


class Entity {
	//std::vector<Component::Base*> components{};
	
	//Ideally, we'd have this use Component::Base::operator<=>. But I can't figure out how.
	//We can't use a function pointer (with decltype). We can't use a lambda either because
	//we're in a header file and thefore an anonymous namespace.
	std::multiset<
		std::unique_ptr<Component::Base>, 
		Component::Base::orderByPriority
	> components {};

public:
	const char* glyph { "￼" };
	Color fgColor { 0xFF0000 };
	uint8_t type { 0 };
	uint8_t zorder { 0 };

	/*
		Add a component to an entity. Returns a handle to that component.
		
		Note: This can't be moved into the .cpp file, because this type of
			function can't be overridden.
	*/
	template<typename T, class ...Args>
	auto add(Args... args)
		requires std::is_base_of<Component::Base, T>::value
	{
		return components.insert(std::make_unique<T>(this, args...));
	}
	
	void rem(auto component) {
		components.erase(component);
	}

	template<typename EventType> //This function must be templated, otherwise the virtual function doesn't get overridden by the correct function.
	EventType dispatch(EventType event)
		requires std::is_base_of<Event::Base, EventType>::value
	{
		//for (auto& c : components) std::cerr << "Component: " << c->name() << "\n";
		for (auto& c : components) c->handleEvent(&event);
		return event;
	}
};