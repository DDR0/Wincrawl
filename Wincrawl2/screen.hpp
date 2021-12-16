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
	
	static bool dirty; //Ideally, only used when the terminal has been resized. Set on screen transition so there's a way back from bad states anyway.
	
	typedef TextCell Cell;
	typedef TextCellGrid OutputGrid;
	static OutputGrid output[2];
	static inline size_t outputBuffer{ 0 }; //Output buffer 0 or 1. The old buffer is used for diffing.
	inline OutputGrid* activeOutputGrid() { return &output[outputBuffer]; }

private:
	void writeOutputToScreen();
protected:
	
	class Panel {
	protected:
		struct Size { int x; int y; };
		struct Offset { int x; int y; };
		bool autowrap {}; ///< Wrap text at the spaces.
	
		Size size {}; ///< Size of the panel itself.
		Offset position {}; ///< Offset of the panel.
		Offset offset {}; ///< Scroll the contents of the panel by x/y, starting from the bottom-left.
	
	public:
		inline Panel(bool autowrap = true) : autowrap(autowrap) {}
		inline void setAutowrap(bool enabled) { autowrap = enabled; }
		inline void setSize(int w, int h) { size.x = w, size.y = h; } //Must be called during initialization.
		inline void setSize(int x, int y, int w, int h) {
			position.x = x, position.y = y;
			size.x = w, size.y = h;
		} 
		
		inline void render(OutputGrid*) { };
	};
	
	class ScrollablePanel : public Panel {
	public:
		inline void setOffset(int x, int y) { offset = {x, y}; }
	};
	
	// TODO: Make this support non-zero positions.
	class CenteredTextPanel : public Panel {
		struct TextLine{ const size_t length; const char* content; };
		typedef std::vector<TextLine> TextBlock;
	
	public:
		const TextBlock text;
		inline CenteredTextPanel(const TextBlock text) : Panel{false}, text{text} {};
		void render(OutputGrid*);
	};
	
	class ViewPanel : public Panel {
	public:
		///Render the view to the view hole.
		void render(OutputGrid* grid, View* view) {
			view->render(getTextCellSubGrid(
				grid, 
				offset.x,
				offset.y,
				offset.x + size.x,
				offset.y + size.y
			));
		};
	};
	
public:
	Triggers triggers {};
	
	Screen(Triggers t) : triggers(t) {};
	virtual ~Screen() = default;
	
	virtual void setSize(size_t x, size_t y);
	virtual void inline render() { writeOutputToScreen(); };
};

class TitleScreen : public Screen {
	CenteredTextPanel text {{
		{ 20, "      [4mWincrawl 0.0.1[0m" },
		{  0, "" },
		{ 20, "n) Enter the Sharded" },
		{ 27, "q) Quit to Operating System" },
	}};

public:
	TitleScreen(Triggers triggers) : Screen(triggers) {
		std::cerr << "TitleScreen constructed.\n";
		setSize(110, 25);
	}
	
	inline void setSize(size_t x, size_t y) override {
		std::cerr << "TitleScreen resized.\n";
		Screen::setSize(x, y);
		text.setSize(0, 0, x, y);
	}
	
	inline void render() override {
		std::cerr << "TitleScreen rendered.\n";
		text.render(activeOutputGrid());
		Screen::render();
	}
	
};

class MainScreen : public Screen {
	ViewPanel viewPanel { false };
	ScrollablePanel memoryPanel { true };
	Panel hintsPanel { true };
	Panel promptPanel { true };
	
	View* view;

public:
	MainScreen(View* view, Triggers triggers) : Screen(triggers), view(view) {
		setSize(110, 25);
	}
	
	/**
	 * Screen Layout
	 * o-----------o-------o
	 * |1↔       0↕|1↔   0↕|
	 * |           |       |
	 * | viewPanel | memry |
	 * |           |       |
	 * |           |       |
	 * o-----------o-------o
	 * |1↔  hintsPanel   1↕|
	 * |1↔  promptPanel  0↕|
	 * o-------------------o
	 * ✥: Stretchiness ratio, like CSS `flex-grow`.
	 */
	inline void setSize(size_t x, size_t y) override {
		Screen::setSize(x, y);
		
		const int gutter = 1;
		const int promptHeight = 2;
		const int interiorWidth = x-gutter*2; int interiorHeight = y-gutter*2;
		const int viewWidth = interiorWidth*1/3;
		const int viewHeight = interiorHeight*1/3 < 4 ? interiorHeight - 4 : interiorHeight*2/3;
		
		viewPanel.setSize(gutter, gutter, viewWidth, viewHeight);
		memoryPanel.setSize(gutter+viewWidth+gutter, gutter, interiorWidth-viewWidth-gutter, viewHeight);
		hintsPanel.setSize(gutter, gutter+viewHeight+gutter, interiorWidth, interiorHeight-viewHeight-gutter-promptHeight);
		promptPanel.setSize(gutter, gutter+viewHeight+gutter, interiorWidth, promptHeight);
	}
	
	inline void render() override {
		//Draw the main view, so we can see the world we're in.
		viewPanel.render(activeOutputGrid(), view);
		memoryPanel.render(activeOutputGrid());
		hintsPanel.render(activeOutputGrid());
		promptPanel.render(activeOutputGrid());
		
		Screen::render();
	}
};

class DeathScreen : public Screen {
	Panel main { false };
	
public:
	inline void setSize(size_t x, size_t y) override {
		Screen::setSize(x, y);
		main.setSize(x, y);
	}
	
	inline DeathScreen(Triggers triggers) : Screen(triggers) {
		setSize(110, 25);
	}
	
};

class CreditsScreen : public Screen {
	Panel main { false };
	
public:
	inline void setSize(size_t x, size_t y) {
		Screen::setSize(x, y);
		main.setSize(x, y);
	}
	inline CreditsScreen(Triggers triggers) : Screen(triggers) {
		setSize(110, 25);
	}
	
};

class DebugScreen : public Screen {
	ScrollablePanel main { true };
	
public:
	inline void setSize(size_t x, size_t y) {
		Screen::setSize(x, y);
		main.setSize(x, y);
	}
	inline DebugScreen(Triggers triggers) : Screen(triggers) {
		setSize(110, 25);
	}
};