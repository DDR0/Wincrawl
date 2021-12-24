#pragma once

/**
 * A std::cerr-like object for debugging, so we can output to Visual Studio's
 * output panel instead of std::cerr which doesn't seem to be redirectable.
 */

#include <sstream>

#ifdef _MSC_VER

	class Debug : public std::stringstream {
		void printToConsole(const char* str);

	public:
		template <class T>
		Debug& operator<<(T&& x) {
			#ifndef NDEBUG
				std::stringstream ss{};
				ss << std::forward<T>(x);
				std::string str = ss.str();
				printToConsole(str.c_str());
			#endif
			return *this;
		};

		template <class T>
		Debug& operator<<(std::ostream& (*manip)(std::ostream&)) {
			#ifndef NDEBUG
				std::stringstream ss{};
				ss << manip;
				std::string str = ss.str();
				printToConsole(str.c_str());
			#endif
			return *this;
		};
	};

#else

	class Debug : public std::stringstream {
	public:
		template <class T>
		Debug& operator<<(T&& x) {
			#ifndef NDEBUG
				std::cerr << std::forward<T>(x);
			#endif
			return *this;
		}

		template <class T>
		Debug& operator<<(std::ostream& (*manip)(std::ostream&)) {
			#ifndef NDEBUG
				std::cerr << manip;
			#endif
			return *this;
		}
	};

#endif