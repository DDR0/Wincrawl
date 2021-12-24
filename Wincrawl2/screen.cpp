#include <cassert>
#include <sstream>
#include <string>
#include <ranges>

#include "color.hpp"
#include "screen.hpp"
#include "seq.hpp"

constexpr auto iota { std::views::iota };
const Color black = Color(0x000000FF);
const Color white = Color(0xFFFFFFFF);
const Color warning = Color(6, 84, 50);
const Color neutralForeground = Color(0xDDDDDDFF);
const Color neutralBackground = Color(0x222222FF);

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
#ifndef NDEBUG
				for (auto& cell : line) {
					cell.character = "X";
					cell.background = warning;
					cell.foreground = white;
				}
#endif
			}
		}
	}
};

void Screen::writeOutputToScreen() {
	static std::string buf {};
	constexpr size_t expectedCharsPerGlyph = 25; //It takes 13 characters to set the foreground, another 13 for the background, and one for the character itself. Plus anything else to move the cursor around.
	buf.reserve(expectedCharsPerGlyph * Screen::size.y * Screen::size.x);
	
	const OutputGrid& newGrid = output[outputBuffer^0];
	const OutputGrid& oldGrid = output[outputBuffer^1];

	assert((
		"Grid resized but not marked dirty.",
		dirty || (newGrid.size() == oldGrid.size() && newGrid[0].size() == oldGrid[0].size() && newGrid[1].size() == oldGrid[1].size())
	));
	
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
				buf.append(std::to_string(y+1)); buf.append(";");
				buf.append(std::to_string(x+1)); buf.append("H");
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



void Screen::Panel::render(Screen::OutputGrid* buffer) {
	assert((
		"Error: Invalid screen size of < 1 character.",
		size.x && size.y
	));

	assert((
		"Incorrect grid size, `position + size <= grid.size()`.\ngg ez",
		(*buffer).size() >= static_cast<size_t>(position.y + size.y) &&
		(*buffer)[0].size() >= static_cast<size_t>(position.x + size.x)
	));

	//"Static Panel", twice, offset, interleaved; further interleaved with spaces; with each character null-terminated.
	const char* panelText = "S\0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0t\0 \0 \0 \0 \0 \0P\0 \0 \0 \0 \0 \0a\0 \0 \0 \0 \0 \0a\0 \0 \0 \0 \0 \0t\0 \0 \0 \0 \0 \0n\0 \0 \0 \0 \0 \0i\0 \0 \0 \0 \0 \0e\0 \0 \0 \0 \0 \0c\0 \0 \0 \0 \0 \0l\0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0P\0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0a\0 \0 \0 \0 \0 \0S\0 \0 \0 \0 \0 \0n\0 \0 \0 \0 \0 \0t\0 \0 \0 \0 \0 \0e\0 \0 \0 \0 \0 \0a\0 \0 \0 \0 \0 \0l\0 \0 \0 \0 \0 \0t\0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0i\0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0c\0 \0 \0 \0 \0 ";
	const size_t panelTextLength = 336 / 2; //Length of panelText, not null-terminated so just hard-coded.
	const size_t panelTextWidth = 2;
	const size_t period = 12;
	const size_t stride = period - 1;

	//Read text in to grid, placing each letter appropriately so the text forms a diagonal.
	for (size_t y : iota(position.y, position.y + size.y)) {
		for (size_t x : iota(position.x, position.x + size.x)) {
			(*buffer)[y][x].character = &panelText[((x*panelTextWidth)+(y*stride*panelTextWidth))%(panelTextLength*panelTextWidth)];
			(*buffer)[y][x].foreground = neutralForeground;
			(*buffer)[y][x].background = neutralBackground;
			(*buffer)[y][x].attributes = 0;
		}
	}
};



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
		"Error: Invalid screen size of < 1 character.",
		size.x && size.y
	));
	
	assert((
		"Incorrect grid size, `position + size <= grid.size()`.\ngg ez",
		(*buffer).size() >= static_cast<size_t>(position.y + size.y) && 
		(*buffer)[0].size() >= static_cast<size_t>(position.x + size.x)
	));
	
	//Flood fill empty. Overdrawn later.
	for (size_t y : iota(position.y, position.y + size.y)) {
		for (size_t x : iota(position.x, position.x + size.x)) {
			(*buffer)[y][x].character = " ";
			(*buffer)[y][x].foreground = neutralForeground;
			(*buffer)[y][x].background = neutralBackground;
			(*buffer)[y][x].attributes = 0;
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
			(*buffer)[y+position.y+topLeft.y][x+position.x+topLeft.x].character = text[y].content;
			while (++x < text[y].length) { //Can't use text[y].length, it's the visual width and not the codepoint count.
				(*buffer)[y+position.y+topLeft.y][x+position.x+topLeft.x].character = "";//text[y].content[x]; //This needs to be transmogrified into individual, cut-up strings. >_<
			}
		}
	}
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
void MainScreen::setSize(size_t x, size_t y) {
	Screen::setSize(x, y);

	const int gutter = 1;
	const int promptHeight = 2;
	const int interiorWidth = x - gutter * 2; int interiorHeight = y - gutter * 2;
	const int viewWidth = interiorWidth * 1 / 3;
	const int viewHeight = interiorHeight * 1 / 3 < 4 ? interiorHeight - 4 : interiorHeight * 2 / 3;

	viewPanel.setSize(gutter, gutter, viewWidth, viewHeight);
	memoryPanel.setSize(gutter + viewWidth + gutter, gutter, interiorWidth - viewWidth - gutter, viewHeight);
	hintsPanel.setSize(gutter, gutter + viewHeight + gutter, interiorWidth, interiorHeight - viewHeight - gutter - promptHeight);
	promptPanel.setSize(gutter, gutter + viewHeight + gutter, interiorWidth, promptHeight);
}

/**
 * Draw the main view, so we can see the world we're in.
 */
void MainScreen::render() {
	auto out = activeOutputGrid();

	renderBorders(); //Do this first so other panels can overwrite it, "explode" out of their frame.
	memoryPanel.render(out);
	hintsPanel.render(out);
	promptPanel.render(out);
	viewPanel.render(out, view); //Most out-of-bounds panel.

	Screen::render();
}

/**
 * Draw the borders of the main screen.
 */
void MainScreen::renderBorders()
{
	constexpr uint_fast16_t zero{ 0 };

	for (auto y : iota(zero + 1, size.y - 1)) {
		writeCell(neutralForeground, neutralBackground, "|", y, 0);
		writeCell(neutralForeground, neutralBackground, "|", y, size.x - 1);
	}

	writeCell(neutralForeground, neutralBackground, "O", 0, 0);
	writeCell(neutralForeground, neutralBackground, "O", 0, size.x - 1);
	writeCell(neutralForeground, neutralBackground, "O", viewPanel.rect()->h + 1, 0);
	writeCell(neutralForeground, neutralBackground, "O", viewPanel.rect()->h + 1, size.x - 1);
	writeCell(neutralForeground, neutralBackground, "O", size.y - 1, size.x - 1);
	writeCell(neutralForeground, neutralBackground, "O", size.y - 1, 0);

	for (auto x : iota(zero + 1, size.x - 1)) {
		writeCell(neutralForeground, neutralBackground, "-", 0, x);
		writeCell(neutralForeground, neutralBackground, "-", viewPanel.rect()->h + 1, x);
		writeCell(neutralForeground, neutralBackground, "-", size.y - 1, x);
	}

	writeCell(neutralForeground, neutralBackground, "O", 0, viewPanel.rect()->w + 1);
	writeCell(neutralForeground, neutralBackground, "O", viewPanel.rect()->h + 1, viewPanel.rect()->w + 1);

	for (auto y : iota(viewPanel.rect()->y, viewPanel.rect()->y + viewPanel.rect()->h)) {
		writeCell(neutralForeground, neutralBackground, "|", y, viewPanel.rect()->w + 1);
	}
}
