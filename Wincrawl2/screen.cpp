#include <cassert>
#include <sstream>
#include <string>
#include <ranges>

#include "color.hpp"
#include "screen.hpp"
#include "seq.hpp"



const auto iota { std::views::iota };

std::vector<std::vector<Screen::Cell>> Screen::output[2] {
	{ Screen::size.y, std::vector<Screen::Cell>{ Screen::size.x } },
	{ Screen::size.y, std::vector<Screen::Cell>{ Screen::size.x } },
};

void Screen::writeOutputToScreen() {
	static std::string buf {};
	constexpr size_t expectedCharsPerGlyph = 25; //It takes 13 characters to set the foreground, another 13 for the background, and one for the character itself. Plus anything else to move the cursor around.
	buf.reserve(Screen::size.y * Screen::size.x * expectedCharsPerGlyph);
	
	OutputGrid newGrid = output[outputBuffer^0];
	OutputGrid oldGrid = output[outputBuffer^1];
	
	static Color lastBackgroundColor { 0x00000000 };
	static Color lastForegroundColor { 0xFFFFFF00 };
	static uint8_t lastAttributes {};
	bool didSkip{ false };
	
	if (dirty) {
		lastAttributes = 0;
		buf.assign(seq::reset); //resets bold, underline, etc. attributes
		buf.append(lastBackgroundColor.bg());
		buf.append(lastForegroundColor.fg());
	} else {
		buf.assign(seq::moveToOrigin);
	}
		
	for (auto y : iota(size_t(0), newGrid.size())) {
		for (auto x : iota(size_t(0), newGrid[0].size())) {
			const Cell& cell = newGrid[y][x];
			const Cell& writ = oldGrid[y][x];
			
			if (!dirty && cell == writ) {
				didSkip = true;
				continue;
			}
			
			if (didSkip) {
				didSkip = false;
				buf.append("\033["); //Escape sequence; move cursor to current row/column.
				buf.append(std::to_string(y)); buf.append(";");
				buf.append(std::to_string(x)); buf.append("H");
			}
			
			if (lastBackgroundColor != cell.background) {
				lastBackgroundColor = cell.background;
				buf.append(cell.background.bg());
			}
			
			if (lastForegroundColor != cell.foreground) {
				lastForegroundColor = cell.foreground;
				buf.append(cell.foreground.bg());
			}
			
			if (lastAttributes != cell.attributes) {
				if ((lastAttributes & cell.bold) ^ (cell.attributes & cell.bold)) {
					buf.append((cell.attributes & cell.bold) ? seq::bold : seq::boldOff);
				}
				if ((lastAttributes & cell.underline) ^ (cell.attributes & cell.underline)) {
					buf.append((cell.attributes & cell.underline) ? seq::underline : seq::underlineOff);
				}
			}
			
			buf.append(cell.character);
		}
		buf.append("\n");
	}
	
	std::cout << buf;
	
	//We want to compare against what we last wrote to screen, so use the other buffer for the new stuff now.
	outputBuffer ^= 1;
	
	//We have now cleaned the screen of any artefacts.
	dirty = false;
}



void Screen::CenteredTextPanel::render(Screen::OutputGrid *buffer) {
	//Center the block of text in the panel.
	size_t height{ text.size() };
	size_t width{ 0 };
	for (auto& line : text) {
		width = std::max(width, line.length);
	};
	
	
	const Offset topLeft {
		size.x/2 - static_cast<int>(width )/2,
		size.y/2 - static_cast<int>(height)/2,
	};
	
	if (size.x < 1 || size.y < 1) {
		std::cerr << "Error: Invalid screen size of < 1 character. Skipping render.\n";
		return assert(false);
	}
	
	buffer->resize(size.y);
	if (buffer->at(0).size() != static_cast<size_t>(size.x)) {
		for (auto& line : *buffer) {
			line.resize(size.x);
		}
	}
	
	
	//Flood fill empty. Slight overdraw but c'est la vie.
	for (size_t y : iota(offset.y, offset.y + size.y)) {
		for (size_t x : iota(offset.x, offset.x + size.x)) {
			(*buffer)[y][x].character = " ";
		}
	}
	
	(*buffer)[0][0].character = "[0m";
	
	//Copy in text. Since text can be variable-width and have escape sequence, etc. we set the
	//first display cell to the text contents and have the rest as zero-width padding. This
	//won't work if we need to scroll the panel, because we actually need to know what character
	//goes where then, but this panel doesn't need that.
	for (size_t y : iota((size_t) 0, height)) {
		if (text[y].length) {
			size_t x = 0;
			(*buffer)[y+offset.y+topLeft.y][x+offset.x+topLeft.x].character = text[y].content;
			while (++x < text[y].length) { //Can't use text[y].length, it's the visual width and not the codepoint count.
				(*buffer)[y+offset.y+topLeft.y][x+offset.x+topLeft.x].character = "";//text[y].content[x]; //This needs to be transmogrified into individual, cut-up strings. >_<
			}
		}
	}
}