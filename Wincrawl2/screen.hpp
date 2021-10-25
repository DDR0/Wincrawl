#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "color.hpp"
#include "ecs.hpp"
#include "triggers.hpp"
#include "view.hpp"

class Screen {
protected:
	struct Size { uint_fast16_t x; uint_fast16_t y; };
	static inline Size size { 80, 25 };
	
	bool dirty{ true }; //Ideally, only used when the terminal has been resized. Set on screen transition so there's a way back from bad states anyway.
	
	struct Cell { //TODO: Test this is 64 bytes.
		const char* character{ "🯄" }; //A one-character-wide utf8 grapheme. Supports combining characters, non-latin unicode, etc.
		Color background {};
		Color foreground {};
		uint8_t attributes {};
		
		constexpr static uint_fast8_t bold      { 1 << 0 };
		constexpr static uint_fast8_t underline { 1 << 1 };
		
		bool operator==(const Cell&) const = default;
	};
	typedef std::vector<std::vector<Cell>> OutputGrid;
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
		bool autowrap {}; //Wrap text at the spaces.
	
		Size size {};
		Offset offset {}; //Scroll the panel by x/y, starting from the bottom-left.
	
	public:
		inline Panel(bool autowrap = true) : autowrap(autowrap) {}
		inline void setAutowrap(bool enabled) { autowrap = enabled; }
		inline void setSize(int x, int y) { size.x = x; size.y = y; } //Must be called during initialization.
		
		inline void render(OutputGrid*) { };
	};
	
	class ScrollablePanel : public Panel {
	public:
		inline void setOffset(int x, int y) { offset = {x, y}; }
	};
	
	class CenteredTextPanel : public Panel {
		struct TextLine{ const size_t length; const char* content; };
		typedef std::vector<TextLine> TextBlock;
	
	public:
		const TextBlock text;
		inline CenteredTextPanel(const TextBlock text) : Panel{false}, text{text} {};
		void render(OutputGrid*);
	};
	
	Screen() {};
	Screen(Triggers t) : triggers(t) {};
	
public:
	Triggers triggers {};
	
	virtual ~Screen() = default;
	
	virtual void inline setSize(int x, int y) {};
	virtual void inline render() { writeOutputToScreen(); };
};

class TitleScreen : public Screen {
	CenteredTextPanel main {{
		{ 20, "      [4mWincrawl 0.0.1[0m" },
		{  0, "" },
		{ 20, "n) Enter the Sharded" },
		{ 27, "q) Quit to Operating System" },
	}};

public:
	inline void setSize(int x, int y) {
		dirty = true;
		size.x = x, size.y = y;
		
		main.setSize(x, y);
	}
	
	inline void render() {
		main.render(activeOutputGrid());
		Screen::render();
	}
	inline TitleScreen(Triggers triggers) : Screen(triggers) {
		setSize(110, 25);
	}
};

class MainScreen : public Screen {
	Panel view { false };
	ScrollablePanel memory { true };
	Panel hints { true };
	Panel prompt { true };

public:
	inline void setSize(int x, int y) override {
		dirty = true;
		size.x = x, size.y = y;
		
		int gutter = 1;
		int promptHeight = 2;
		int interiorWidth = x-gutter*2; int interiorHeight = y-gutter*2;
		int viewWidth = interiorWidth*2/3;
		int viewHeight = interiorHeight/2 - promptHeight/2;
		
		view.setSize(viewWidth, viewHeight);
		memory.setSize(interiorWidth-viewWidth-gutter, viewHeight);
		hints.setSize(interiorWidth, interiorHeight-viewHeight-gutter);
		prompt.setSize(interiorWidth, promptHeight);
	}
	inline MainScreen() { setSize(110, 25); }
};

class DeathScreen : public Screen {
	Panel main { false };
	
public:
	inline void setSize(int x, int y) {
		dirty = true;
		size.x = x, size.y = y;
		
		main.setSize(x, y);
	}
	inline DeathScreen() { setSize(110, 25); }
	
};

class CreditsScreen : public Screen {
	Panel main { false };
	
public:
	inline void setSize(int x, int y) {
		dirty = true;
		size.x = x, size.y = y;
		
		main.setSize(x, y);
	}
	inline CreditsScreen() { setSize(110, 25); }
	
};

class DebugScreen : public Screen {
	ScrollablePanel main { true };
	
public:
	inline void setSize(int x, int y) {
		dirty = true;
		size.x = x, size.y = y;
		
		main.setSize(x, y);
	}
	inline DebugScreen() { setSize(110, 25); }
};