//Encapsulate https://www.hsluv.org/ in a convenient, over-engineered wrapper.
#pragma once

#include <string>
#include <cstdint>

#include "hsluv.hpp"

class Color {
	uint8_t channels[4]{}; //Note: Occasionally used as a uint32_t too.
	
public:
	struct RGBA { uint8_t r; uint8_t g; uint8_t b; uint8_t a; };
	struct RGB { uint8_t r; uint8_t g; uint8_t b; };
	struct HSLA { double h; double s; double l; double a; };
	struct HSL { double h; double s; double l; };
	
	Color();
	Color(uint32_t);
	Color(RGB);
	Color(RGBA);
	Color(HSL);
	Color(HSLA);
	Color(double h, double s, double l);
	Color(double h, double s, double l, double a);
	Color(const Color&);
	
	inline int a() { return channels[3]; };

	void rgb(uint8_t r, uint8_t g, uint8_t b);
	RGB rgb() const;
	void rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	RGBA rgba() const;
	
	void hsl(double h, double s, double l);
	HSL hsl() const;
	void hsla(double h, double s, double l, double a);
	HSLA hsla() const;

	Color& operator=(const Color&);
	Color& operator=(uint32_t rgba);
	
	bool operator==(const Color&) const;

	uint8_t operator[](size_t);
	uint8_t operator[](size_t) const;
	
	friend auto operator<<(std::ostream& os, Color const& color) -> std::ostream&;
	
	const std::string fg() const;
	const std::string bg() const;
};