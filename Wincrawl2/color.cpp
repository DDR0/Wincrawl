//Encapsulate https://www.hsluv.org/ in a convenient, over-engineered wrapper.

#include <cassert>
#include <cmath>
#include <iostream>

#include "color.hpp"
	
Color::Color() : channels{0, 0, 0} {}

Color::Color(uint32_t rgb) : channels { 
	static_cast<uint8_t>(rgb>>16),
	static_cast<uint8_t>(rgb>>8&0xFF),
	static_cast<uint8_t>(rgb&0xFF)
} { assert(rgb <= 0xffffff); }

Color::Color(RGB other) : channels{ other.r, other.g, other.b } {}

Color::Color(double h, double s, double l) {
	double channelsIn[3];
	hsluv2rgb(h, s, l,
		&channelsIn[0], &channelsIn[1], &channelsIn[2]);
	
	for (int i = 0; i < 3; ++i)
		channels[i] = int(std::round(channelsIn[i]*255));
}

Color::Color(HSL other) {
	double channelsIn[3];
	hsluv2rgb(other.h, other.s, other.l,
		&channelsIn[0], &channelsIn[1], &channelsIn[2]);
	
	for (int i = 0; i < 3; ++i)
		channels[i] = int(std::round(channelsIn[i]*255));
}
Color::Color(const Color& other) : channels{ other[0], other[1], other[2] } {}

void Color::rgb(uint8_t r, uint8_t g, uint8_t b) {
	channels[0] = r;
	channels[1] = g;
	channels[2] = b;
}

void Color::hsl(double h, double s, double l) {
	double channelsIn[3];
	hsluv2rgb(h, s, l,
		&channelsIn[0], &channelsIn[1], &channelsIn[2]);
	
	for (int i = 0; i < 3; ++i)
		channels[i] = int(std::round(channelsIn[i]*255));
}

Color::RGB Color::rgb() const {
	return RGB(channels[0], channels[1], channels[2]);
}

Color::HSL Color::hsl() const {
	HSL newcolour;
	rgb2hsluv(
		channels[0]/255., channels[1]/255., channels[2]/255., 
		&newcolour.h, &newcolour.s, &newcolour.l
	);
	return newcolour;
}

Color& Color::operator=(const Color& other) {
	for (int i = 0; i < 3; i++)
		channels[i] = other[i];
	return *this;
}

Color& Color::operator=(uint32_t rgb) {
	assert(rgb <= 0xffffff);
	channels[0] = static_cast<uint8_t>(rgb>>16);
	channels[1] = static_cast<uint8_t>(rgb>>8&0xFF);
	channels[2] = static_cast<uint8_t>(rgb&0xFF);
	return *this;
}

uint8_t Color::operator[](size_t i) {
	assert(i < 3);
	return channels[i];
}

uint8_t Color::operator[](size_t i) const {
	assert(i < 3);
	return channels[i];
}

auto operator<<(std::ostream& os, Color const& color) -> std::ostream& {
	const Color::HSL hsl = color.hsl();
	return os
		<< "HSLuv(" //display values
		<< std::to_string(int(std::round(hsl.h))) << ","
		<< std::to_string(int(std::round(hsl.s))) << ","
		<< std::to_string(int(std::round(hsl.l))) << ")"

		<< "[38;2;" //display color
		<< std::to_string(color[0]) << ";"
		<< std::to_string(color[1]) << ";"
		<< std::to_string(color[2]) << "m﹅[0m";
}

const std::string Color::fg() const {
	return "[38;2;"
		+ std::to_string(this->channels[0]) + ";"
		+ std::to_string(this->channels[1]) + ";"
		+ std::to_string(this->channels[2]) + "m";
}

const std::string Color::bg() const {
	return "[48;2;"
		+ std::to_string(this->channels[0]) + ";"
		+ std::to_string(this->channels[1]) + ";"
		+ std::to_string(this->channels[2]) + "m";
}