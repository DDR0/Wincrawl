//Classes related to things, entities, on the map.
#pragma once

#include <type_traits>
#include <memory>

#include "color.hpp"
#include "places.hpp"
#include "vector_tools.hpp" //Only needed by rem(Component::Base).

class Tile; //Not defined by places.hpp, as places.hpp requires this first.
class Entity;

struct Event {
	//An event is something which happens. Eg., you stab something or get stabbed, you are drawn on screen, etc.

	class Base {};

	class Damage : public Event::Base {
	public:
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
	class TakeDamage : public Event::Damage {};
	class DoAttack : public Event::Damage {};

	class GetRendered : public Event::Base {
	public:
		const char* glyph{ nullptr };
		Color fgColor{ 0xFF0000 };
		Color bgColor{ 0xFF0000 }; //needs some concept of an alpha channel, so disused for now
	};
};


struct Component {
	enum class priority { last, unimportant, neutral, important, veryImportant, first };

	class Base {
	public:
		static const auto priority{ Component::priority::unimportant };

		Entity* entity{ nullptr };
		
		Base(Entity*);

		//Specializations are needed, otherwise base just gets called.
		virtual void handleEvent(Event::TakeDamage*) {};
		virtual void handleEvent(Event::DoAttack*) {};
		virtual void handleEvent(Event::GetRendered*) {};
	};

	class Health : public Component::Base {
	public:
		static const auto priority{ Component::priority::neutral };
		int hp{ 10 };

		Health(Entity*, int hp);

		void handleEvent(Event::TakeDamage*) override;
		void handleEvent(Event::DoAttack*) override;
	};

	class Render : public Component::Base {
	public:
		const char* glyph{ "￼" };
		Color fgColor{ 0xFF0000 };
		uint8_t type{ 0 };
		uint8_t zorder{ 0 };

		Render(Entity*, const char* glyph_, Color color_);

		void handleEvent(Event::GetRendered*) override;
	};
};


class Entity {
	//std::vector<Component::Base*> components{};
	std::vector<std::unique_ptr<Component::Base>> components{};

public:
	const char* glyph{ "￼" };
	Color fgColor{ 0xFF0000 };
	uint8_t type { 0 };
	uint8_t zorder { 0 };

	//TODO: Figure out how to get this into the .cpp file.
	template<typename T, class ...Args>
	void add(Args... args)
		requires std::is_base_of<Component::Base, T>::value
	{
		components.push_back(std::make_unique<T>(this, args...));
	}

	template<typename T>
	void rem()
		requires std::is_base_of<Component::Base, T>::value
	{
		//???
		throw "unimplimented";
	};

	template<typename EventType> //This function must be templated, otherwise the virtual function doesn't get overridden by the correct function.
	EventType* dispatch(EventType* event)
		requires std::is_base_of<Event::Base, EventType>::value
	{
		for (auto& c : components) c->handleEvent(event);
		return event;
	}
};