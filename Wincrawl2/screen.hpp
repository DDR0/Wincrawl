#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "color.hpp"
#include "debug.hpp"
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
	OutputGrid* activeOutputGrid() { return &output[outputBuffer]; }

private:
	void writeOutputToScreen();
protected:
	Debug cerr{};

	class Panel {
	protected:
		Debug cerr{};

		struct Offset { int x; int y; };
		struct Size { int x; int y; };
		bool autowrap {}; ///< Wrap text at the spaces.
	
		Offset position {}; ///< Offset of the panel.
		Size size{}; ///< Size of the panel itself. Must come after position, for getSize.
		Offset offset {}; ///< Scroll the contents of the panel by x/y, starting from the bottom-left.
	
	public:
		Panel(bool autowrap = true) : autowrap(autowrap) {}
		void setAutowrap(bool enabled) { autowrap = enabled; }
		void setSize(int w, int h) { size.x = w, size.y = h; }
		void setSize(int x, int y, int w, int h) {
			position.x = x, position.y = y;
			size.x = w, size.y = h;
		}
		struct xywhRect { int x; int y; int w; int h; };
		xywhRect* rect() { return reinterpret_cast<xywhRect*>(&position); }
		
		void render(OutputGrid*);
	};
	
	class ScrollablePanel : public Panel {
	public:
		inline void setOffset(int x, int y) { offset = {x, y}; }
	};
	
	// TODO: Make this support non-zero positions.
	class CenteredTextPanel : public Panel {
		struct TextLine { const size_t length; const char* content; };
		typedef std::vector<TextLine> TextBlock;

	public:
		const TextBlock text;
		CenteredTextPanel(const TextBlock text) : Panel{ false }, text{ text } {};
		void render(OutputGrid*);
	};
	
	class ViewPanel : public Panel {
	public:
		///Render the view to the view hole.
		void render(OutputGrid* grid, View* view) {
			view->render(getTextCellSubGrid(
				grid, 
				position.x,
				position.y,
				position.x + size.x,
				position.y + size.y
			));
		};
	};

	Screen& writeCell(Color fg, Color bg, const char* character, size_t y, size_t x, int attrs=0) {
		Cell& cell = (*activeOutputGrid())[y][x];
		cell.character = character;
		cell.background = bg;
		cell.foreground = fg;
		cell.attributes = attrs;
		return (*this);
	}
	
public:
	Triggers triggers {};
	
	Screen(Triggers t) : triggers(t) {};
	virtual ~Screen() = default;
	
	virtual void setSize(size_t x, size_t y);
	virtual void render(const char* input) { writeOutputToScreen(); };
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
	}
	
	void setSize(size_t x, size_t y) override {
		Screen::setSize(x, y);
		text.setSize(0, 0, x, y);
	}
	
	void render(const char* input) override {
		text.render(activeOutputGrid());
		Screen::render(input);
	}
	
};

class MainScreen : public Screen {
	ViewPanel viewPanel { false };
	ScrollablePanel memoryPanel { true };
	Panel hintsPanel { true };
	Panel promptPanel { true };
	
	View* view;

	void renderBorders();
	void renderPromptPanel(const char* input);

public:
	MainScreen(View* view, Triggers triggers) : Screen(triggers), view(view) {
	}
	
	void setSize(size_t x, size_t y) override;
	void render(const char* input) override;
};

class DeathScreen : public Screen {
	Panel main { false };
	
public:
	void setSize(size_t x, size_t y) override {
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
	void setSize(size_t x, size_t y) {
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
	void setSize(size_t x, size_t y) {
		Screen::setSize(x, y);
		main.setSize(x, y);
	}
	inline DebugScreen(Triggers triggers) : Screen(triggers) {
		setSize(110, 25);
	}
};