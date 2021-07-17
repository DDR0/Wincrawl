//Encapsulate https://www.hsluv.org/ in a convenient, over-engineered wrapper.
#pragma once

#include <string>
#include <cstdint>

#include "hsluv.hpp"

class Color {
	uint8_t channels[3]{};
	
public:
	struct RGB { uint8_t r; uint8_t g; uint8_t b; };
	struct HSL { double h; double s; double l; };
	
	Color();
	Color(uint32_t);
	Color(RGB);
	Color(HSL);
	Color(double h, double s, double l);
	Color(const Color&);

	void rgb(uint8_t r, uint8_t g, uint8_t b);
	RGB rgb() const;
	
	void hsl(double h, double s, double l);
	HSL hsl() const;

	Color& operator=(const Color&);
	Color& operator=(uint32_t rgb);

	uint8_t operator[](size_t);
	uint8_t operator[](size_t) const;
	
	friend auto operator<<(std::ostream& os, Color const& color) -> std::ostream&;
	
	const std::string fg() const;
	const std::string bg() const;
};