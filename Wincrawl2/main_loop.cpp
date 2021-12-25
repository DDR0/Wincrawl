#include <cassert>
#include <chrono>
#include <iostream>
#include <atomic>
#include <thread>

#include "main_loop.hpp"
#include "debug.hpp"
#include "io.hpp"

#ifdef _MSC_VER
#include <immintrin.h>
//unsigned int _lzcnt_u32 (unsigned int a)
#define COUNT_LEADING_ZEROS _lzcnt_u32
#else
//int __builtin_clz (unsigned int x)
#define COUNT_LEADING_ZEROS __builtin_clz
#endif

Debug cerr_{};

bool stopMainLoop{ false };
void runMainLoop(std::shared_ptr<Screen>& screen) {
	using namespace std::chrono_literals;
	
	std::atomic<int> chr{ -1 };
	std::jthread inputListener(getInputCharAsync, ref(chr));

	const int maxInputBufferLength = 8;
	int inputBuffered{ 0 };
	char inputBuffer[maxInputBufferLength]{};

	//cerr_ << "> ";
	while (true) {
		auto nextFrame = std::chrono::steady_clock::now() + 16ms;
		int chr_ = chr.load();
		if (chr_ >= 0) {
			#ifdef _MSC_VER
				if (!chr_) chr_++; //Windows sends the sequence [0, 160] for alt-arrow-keys. wtf, why, omg, just add one if we get it on that platform. >_<
			#endif

			//chr_ is, presumably, a utf32 character. We use utf8 internally. Convert. [Note: This is wrong.]
			auto codepoints{ 4 - COUNT_LEADING_ZEROS(chr_) / 8 };
			inputBuffered += codepoints;
			assert(inputBuffered + 1 < maxInputBufferLength); //Last char must be reserved for 0.
			for (; codepoints; codepoints--) {
				inputBuffer[inputBuffered - codepoints] =
					(chr_ >> 8 * (codepoints - 1)) & 0xFF;
			}
			
			//Better output, just shows characters entered so far.
			//cerr_ << "        \r> "; //maxInputBufferLength spaces
			for (int i=0; inputBuffer[i]; i++) {
				if (inputBuffer[i] < ' ') { //print symbol for control character so we can see it
					std::string symbol{ "␀" };
					symbol[2] += inputBuffer[i];
					//cerr_ << symbol;
				} else {
					//cerr_ << inputBuffer[i];
				}
			}
			
			//Debug output, includes hex and buffer.
			//cerr_ << "key code 0x" << std::hex << chr_ << " " << (char)chr_ << " (" << reinterpret_cast<const char*>(inputBuffer) << ")" << "\n";
			
			if (!screen->triggers.run(inputBuffer)) {
				inputBuffered = 0;
				for (int i = 0; i < maxInputBufferLength; i++)
					inputBuffer[i] = 0;
				//cerr_ << "\r>         \r> "; //maxInputBufferLength spaces, clear any buffer there and reset cursor to where it should be.
			}
			
			if (!chr_ || chr_ == 4 || stopMainLoop) { //null or ctrl-d
				chr = getInputCharAsync::stop;
				break;
			}
			else {
				chr = getInputCharAsync::next; //Next char.
			}
		}
		screen->render();
		std::this_thread::sleep_until(nextFrame);
	}
}