//Classes related to things, entities, on the map.
#pragma once

#include <type_traits>

#include "color.hpp"
#include "places.hpp"
#include "vector_tools.hpp" //Only needed by remComponent(...).

class Tile; //Not defined by places.hpp, as places.hpp requires this first.
class Entity;

struct Event {
	//An event is something which happens. Eg., you stab something or get stabbed, you are drawn on screen, etc.

	class Base {
		template<typename EventType>
		static void dispatch(Entity*, EventType*)
			requires std::is_base_of<Event::Base, EventType>::value;
	};

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
		inline Damage(int amount_, bool physical = true) : amount(amount_) {
			type.physical = physical;
		};
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

		virtual void handleEvent(Event::Base*);
	};

	class Health : public Component::Base {
	public:
		static const auto priority{ Component::priority::neutral };
		int hp{ 10 };

		Health(Entity*, int hp);

		inline void handleEvent(Event::Base*) {};
		void handleEvent(Event::TakeDamage*);
		void handleEvent(Event::DoAttack*);
	};

	class Render : public Component::Base {
	public:
		const char* glyph{ "￼" };
		Color fgColor{ 0xFF0000 };
		uint8_t type{ 0 };
		uint8_t zorder{ 0 };

		Render(Entity*, const char* glyph_, Color color_);

		inline void handleEvent(Event::Base*) {};
		void handleEvent(Event::GetRendered*);
	};
};


class Entity {
	std::vector<Component::Base*> components{};

public:
	const char* glyph{ "￼" };
	Color fgColor{ 0xFF0000 };
	uint8_t type { 0 };
	uint8_t zorder { 0 };

	//TODO: Figure out how to get this into the .cpp file.
	template<typename T, class ...Args>
	inline void addComponent(Args... args)
		requires std::is_base_of<Component::Base, T>::value
	{
		components.push_back(new T(this, args...));
	};

	template<typename T>
	inline void remComponent()
		requires std::is_base_of<Component::Base, T>::value
	{
		//???
		throw "unimplimented";
	};
};


