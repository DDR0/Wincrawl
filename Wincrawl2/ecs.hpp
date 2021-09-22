//Classes related to things, entities, on the map.
#pragma once

#include <type_traits>
#include <memory>
#include <set>
#include <vector>

#include "color.hpp"
#include "vector_tools.hpp" //Only needed by rem(Component::Base).

class Entity;

namespace Event {
	//An event is something which happens. Eg., you stab something or get stabbed, you are drawn on screen, etc.

	struct Base {
	};

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
	
	struct GetBaseHP: Base {};

	struct GetRendered: Base {
		const char* glyph{ nullptr };
		Color fgColor{ 0xFF0000 }; //Rename these to primaryColor and secondaryColor?
		Color bgColor{ 0xFF0000 }; //Needs some concept of an alpha channel, so disused for now.
	};
	
	struct BaseEntityEvent: Base {
		Entity* entity;
		bool force{ false }; //Event will succeed if a handler exists.
		bool success{ false };
	};
	struct AddSubentity: public BaseEntityEvent {};
	struct RemoveSubentity: public BaseEntityEvent {};
	struct MoveTo: public BaseEntityEvent {};
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
		virtual ~Base() = default; //Let delete remove the correct subclass when unloading a Plane of existence.
		
		//Ideally, we'd figure out a way to make orderByPriority() use this, but I can't get it to work.
		std::weak_ordering operator<=>(const Base& other) const {
			return priority() <=> other.priority();
		};

		//Specializations are needed, otherwise base just gets called.
		inline virtual void handleEvent(Event::TakeDamage*) {};
		inline virtual void handleEvent(Event::DealDamage*) {};
		inline virtual void handleEvent(Event::GetRendered*) {};
		inline virtual void handleEvent(Event::AddSubentity*) {};
		inline virtual void handleEvent(Event::RemoveSubentity*) {};
		inline virtual void handleEvent(Event::MoveTo*) {};
	};

	class Existance : public Base {
	public:
		const char* name() const override { return "Existance"; };
		
		//Visually exist.
		const char* glyph{ "￼" };
		Color fgColor{ 0xFF0000 };
		uint8_t type{ 0 };
		uint8_t zorder{ 0 };
		
		//Physically exist, hold and be held.
		Entity* superentity { nullptr };
		std::set<Entity*> subentities {};

		inline Existance(Entity* e, const char* glyph_, Color color, Entity* superentity_ = nullptr)
			: Base(e), glyph(glyph_), fgColor(color), superentity(superentity_) {}

		void handleEvent(Event::GetRendered*) override;
		
		void handleEvent(Event::AddSubentity*) override;
		void handleEvent(Event::RemoveSubentity*) override;
		void handleEvent(Event::MoveTo*) override;
	};

	class Fragility : public Base {
	public:
		const char* name() const override { return "Fragility"; };
		
		int hp{ 10 };

		inline Fragility(Entity* e, int startingHP): Base(e), hp(startingHP) {}

		void handleEvent(Event::TakeDamage*) override;
		void handleEvent(Event::DealDamage*) override; //temp, I think
	};
};


class Entity {
	//std::vector<Component::Base*> components{};
	
	struct orderComponentsByPriority {
		bool operator() (const auto& lhs, const auto& rhs) const {
			return lhs->priority() < rhs->priority();
		}
	};
	
	//Ideally, we'd have this use Component::Base::operator<=>. But I can't figure out how.
	//We can't use a function pointer (with decltype). We can't use a lambda either because
	//we're in a header file and thefore an anonymous namespace.
	std::multiset<
		std::unique_ptr<Component::Base>, 
		Entity::orderComponentsByPriority
	> components {};

public:
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