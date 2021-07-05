#include <cassert>
#include <chrono>
#include <iostream>
#include <atomic>
#include <thread>

#include "main_loop.hpp"
#include "io.hpp"

#ifdef _MSC_VER
//unsigned int _lzcnt_u32 (unsigned int a)
#include "immintrin.h"
#define COUNT_LEADING_ZEROS _lzcnt_u32
#else
//int __builtin_clz (unsigned int x)
#define COUNT_LEADING_ZEROS __builtin_clz
#endif

bool stopMainLoop{ false };
void runMainLoop(Triggers& triggers) {
	using namespace std::chrono_literals;
	
	std::atomic<int> chr{ -1 };
	std::jthread inputListener(getInputCharAsync, ref(chr));

	const int maxInputBufferLength = 8;
	int inputBuffered{ 0 };
	char inputBuffer[maxInputBufferLength]{};

	std::cout << "> ";
	while (true) {
		auto nextFrame = std::chrono::steady_clock::now() + 16ms;
		int chr_ = chr.load();
		if (chr_ >= 0) {
			//chr_ is, presumably, a utf32 character. We use utf8 internally. Convert.
			auto codepoints{ 4 - COUNT_LEADING_ZEROS(chr_) / 8 };
			inputBuffered += codepoints;
			assert(inputBuffered + 1 < maxInputBufferLength); //Last char must be reserved for 0.
			for (; codepoints; codepoints--)
				inputBuffer[inputBuffered - codepoints] =
				(chr_ >> 8 * (codepoints - 1)) & 0xFF;
			
			//Better output, just shows characters entered so far.
			std::cout << "        \r> "; //maxInputBufferLength spaces
			for (int i=0; inputBuffer[i]; i++) {
				if (inputBuffer[i] < ' ') { //print symbol for control character so we can see it
					std::string symbol{ "␀" };
					symbol[2] += inputBuffer[i];
					std::cout << symbol;
				} else {
					std::cout << inputBuffer[i];
				}
			}
			
			//Debug output, includes hex and buffer.
			//std::cout << std::hex << chr_ << " " << (char)chr_ << " (" << reinterpret_cast<const char*>(inputBuffer) << ")" << "\n> ";
			
			if (!triggers.run(inputBuffer)) {
				inputBuffered = 0;
				for (int i = 0; i < maxInputBufferLength; i++)
					inputBuffer[i] = 0;
				std::cout << "\r>         \r> "; //maxInputBufferLength spaces, clear any buffer there and reset cursor to where it should be.
			}


			if (!chr_ || chr_ == 4 || stopMainLoop) { //Null, ctrl-d.
				chr = getInputCharAsync::stop;
				break;
			}
			else {
				chr = getInputCharAsync::next; //Next char.
			}
		}
		std::this_thread::sleep_until(nextFrame);
	}
}