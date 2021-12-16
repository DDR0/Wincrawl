#include <cassert>
#include <sstream>
#include <string>
#include <ranges>

#include "color.hpp"
#include "screen.hpp"
#include "seq.hpp"

constexpr auto iota { std::views::iota };
const Color neutralForeground = Color(0xB0BBBBFF);
const Color neutralBackground = Color(0x444644FF);

bool Screen::dirty{ true };
std::vector<std::vector<Screen::Cell>> Screen::output[2] {
	{ Screen::size.y, std::vector<Screen::Cell>{ Screen::size.x } },
	{ Screen::size.y, std::vector<Screen::Cell>{ Screen::size.x } },
};

void Screen::setSize(size_t x, size_t y) {
	if (output[0].size() != output[1].size() || output[0][0].size() != output[1][0].size()) {
		throw("Buffer size mismatch.");
	}
	
	dirty = true;
	size.x = x, size.y = y;
	if (output[0].size() != y || output[0][0].size() != x) {
		for (auto& buffer : output) {
			buffer.resize(y);
			for (auto& line : buffer) {
				line.resize(x);
			}
		}
	}
};

void Screen::writeOutputToScreen() {
	static std::string buf {};
	constexpr size_t expectedCharsPerGlyph = 25; //It takes 13 characters to set the foreground, another 13 for the background, and one for the character itself. Plus anything else to move the cursor around.
	buf.reserve(expectedCharsPerGlyph * Screen::size.y * Screen::size.x);
	
	OutputGrid newGrid = output[outputBuffer^0];
	OutputGrid oldGrid = output[outputBuffer^1];

	assert((
		"Grid resized but not marked dirty.",
		dirty || (newGrid.size() == oldGrid.size() && newGrid[0].size() == oldGrid[0].size() && newGrid[1].size() == oldGrid[1].size())
	));
	
	////////////////// hack, patching render mode is broken
	dirty = true;
	////////////////// hack
	
	static Color lastBackgroundColor { 0x00000000 };
	static Color lastForegroundColor { 0xFFFFFF00 };
	static uint8_t lastAttributes {};
	bool didSkip{ false };
	
	//Reset cursor.
	buf.assign(seq::moveToOrigin);
	if (dirty) {
		lastAttributes = 0;
		buf.append(seq::reset); //resets bold, underline, etc. attributes
	}
	buf.append(lastBackgroundColor.bg());
	buf.append(lastForegroundColor.fg());

	for (auto y : iota(size_t(0), newGrid.size())) {
		for (auto x : iota(size_t(0), newGrid[0].size())) {
			const Cell& cell = newGrid[y][x];

			if (!dirty && cell == oldGrid[y][x]) { //Note: oldGrid is only dereferencable when clean; otherwise its size might be smaller than newGrid. (This includes taking a reference to it.)
				//If the current cell is the same as the old cell, skip writing it. This is solely to cut down on overdraw.
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
				buf.append(cell.foreground.fg());
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
	
	std::cout << buf << neutralForeground.fg() << neutralBackground.bg();
	
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
	
	assert((
		"Error: Invalid screen size of < 1 character. Skipping render.",
		size.x && size.y
	));
	
	assert((
		"Incorrect grid size, `offset + size <= grid.size()`.\ngg ez",
		(*buffer).size() >= static_cast<size_t>(offset.y + size.y) && 
		(*buffer)[0].size() >= static_cast<size_t>(offset.x + size.x)
	));
	
	//Flood fill empty. Overdrawn later but oh well.
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