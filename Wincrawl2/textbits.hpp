#pragma once

#include <span>
#include <vector>
#include <memory>

#include "color.hpp"

/// Holds characters with attributes for printing, colour, bold, and underline.
struct TextCell { //TODO: Test this is 64 bytes.
	const char* character{ "🯄" }; ///< A one-character-wide utf8 grapheme. Supports combining characters, non-latin unicode, etc.
	Color foreground {};
	Color background{};
	uint8_t attributes {};
	
	constexpr static uint_fast8_t bold      { 1 << 0 };
	constexpr static uint_fast8_t underline { 1 << 1 };
	
	bool operator==(const TextCell&) const = default;
};

/// Vector-based 2d array of TextCells.
typedef std::vector<std::vector<TextCell>> TextCellGrid;

/// Reference to a sub-portion of TextCellGrid.
typedef std::vector<std::span<TextCell>> TextCellSubGrid;

/**
 * Returns a new subgrid via vector of spans to the original subgrid.
 * 
 * Note: Subgrid is invalidated upon iterator invalidation of the original grid.
 */
std::unique_ptr<TextCellSubGrid> getTextCellSubGrid(
	TextCellGrid* grid, size_t x1, size_t y1, size_t x2, size_t y2);