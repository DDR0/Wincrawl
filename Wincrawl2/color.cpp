//Encapsulate https://www.hsluv.org/ in a convenient, over-engineered wrapper.

#include <cassert>
#include <cmath>
#include <iostream>

#include "color.hpp"
	
Color::Color() : channels{0, 0, 0, 1} {}

Color::Color(uint32_t rgba) : channels { 
	static_cast<uint8_t>(rgba>>24     ),
	static_cast<uint8_t>(rgba>>16&0xFF),
	static_cast<uint8_t>(rgba>> 8&0xFF),
	static_cast<uint8_t>(rgba    &0xFF)
} {}

Color::Color(RGB other) : channels{ other.r, other.g, other.b, 1 } {}
Color::Color(RGBA other) : channels{ other.r, other.g, other.b, other.a } {}

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
Color::Color(HSLA other) {
	double channelsIn[4];
	hsluv2rgb(other.h, other.s, other.l,
		&channelsIn[0], &channelsIn[1], &channelsIn[2]);
	channelsIn[3] = other.a;
	
	for (int i = 0; i < 4; ++i)
		channels[i] = int(std::round(channelsIn[i]*255));
}
Color::Color(const Color& other) : channels{ other[0], other[1], other[2], other[3] } {}

void Color::rgb(uint8_t r, uint8_t g, uint8_t b) {
	channels[0] = r;
	channels[1] = g;
	channels[2] = b;
}

void Color::rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	channels[0] = r;
	channels[1] = g;
	channels[2] = b;
	channels[3] = a;
}

void Color::hsl(double h, double s, double l) {
	double channelsIn[3];
	hsluv2rgb(h, s, l,
		&channelsIn[0], &channelsIn[1], &channelsIn[2]);
	
	for (int i = 0; i < 3; ++i)
		channels[i] = int(std::round(channelsIn[i]*255));
}

void Color::hsla(double h, double s, double l, double a) {
	double channelsIn[4];
	hsluv2rgb(h, s, l,
		&channelsIn[0], &channelsIn[1], &channelsIn[2]);
	channelsIn[3] = a;
	
	for (int i = 0; i < 4; ++i)
		channels[i] = int(std::round(channelsIn[i]*255));
}

Color::RGB Color::rgb() const {
	return RGB(channels[0], channels[1], channels[2]);
}

Color::RGBA Color::rgba() const {
	return RGBA(channels[0], channels[1], channels[2], channels[3]);
}

Color::HSL Color::hsl() const {
	HSL newcolour;
	rgb2hsluv(
		channels[0]/255., channels[1]/255., channels[2]/255., 
		&newcolour.h, &newcolour.s, &newcolour.l
	);
	return newcolour;
}

Color::HSLA Color::hsla() const {
	HSLA newcolour;
	rgb2hsluv(
		channels[0]/255., channels[1]/255., channels[2]/255., 
		&newcolour.h, &newcolour.s, &newcolour.l
	);
	newcolour.a = channels[3]/255.;
	return newcolour;
}

Color& Color::operator=(const Color& other) {
	for (int i = 0; i < 4; i++) //bump to 4 to match alpha too
		channels[i] = other[i];
	return *this;
}

Color& Color::operator=(uint32_t rgba) {
	channels[0] = static_cast<uint8_t>(rgba>>24);
	channels[1] = static_cast<uint8_t>(rgba>>16&0xFF);
	channels[2] = static_cast<uint8_t>(rgba>> 8&0xFF);
	channels[3] = static_cast<uint8_t>(rgba    &0xFF);
	return *this;
}

uint8_t Color::operator[](size_t i) {
	assert(i < 4);
	return channels[i];
}

uint8_t Color::operator[](size_t i) const {
	assert(i < 4);
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