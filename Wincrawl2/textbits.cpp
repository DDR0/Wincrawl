#include <cassert>
#include <iostream>
#include <ranges>

#include "textbits.hpp"

const auto iota { std::views::iota };

std::unique_ptr<TextCellSubGrid> getTextCellSubGrid(
	TextCellGrid* grid, size_t x1, size_t y1, size_t x2, size_t y2) {
	//std::cerr << x1 << " " << y1 << " " << x2 << " " << y2 << " in " << grid->size() << " " << (*grid)[0].size() << "\n"; 
	assert(x1 <= x2);
	assert(y1 <= y2);
	assert(y2 <= grid->size());
	if(grid->size()) {
		assert(x2 <= (*grid)[0].size());
	} else {
		return std::make_unique<TextCellSubGrid>();
	}
	
	auto subgrid { std::make_unique<TextCellSubGrid>() };
	subgrid->reserve(y2-y1);
	
	for (auto y : iota(y1, y2)) {
		subgrid->emplace_back(
			(*grid)[y].begin()+x1, 
			(*grid)[y].begin()+x2
		);
	}
	return subgrid;
}