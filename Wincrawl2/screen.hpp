#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "color.hpp"
#include "ecs.hpp"
#include "textbits.hpp"
#include "triggers.hpp"
#include "view.hpp"

/**
 * Screens are the different views of our application. Each screen has at least
 * one panel in it, which shows content. For example, the main screen shows the
 * view panel, granting us a window into the world as we navigate it.
 */
class Screen {
protected:
	struct Size { uint_fast16_t x; uint_fast16_t y; };
	static inline Size size { 80, 25 };
	
	bool dirty{ true }; //Ideally, only used when the terminal has been resized. Set on screen transition so there's a way back from bad states anyway.


protected:
	class ViewPanel {
	public:
		void render(View view) {
			view.render();
		};
	};
	
public:
	Triggers triggers {};
	
	Screen(Triggers t) : triggers(t) {};
	virtual ~Screen() = default;
};



class MainScreen : public Screen {
public:
	MainScreen(View& view) : Screen(Triggers{{}}) {
		ViewPanel viewPanel { };
		//viewPanel.render(view);
		view.render();
	}
};

