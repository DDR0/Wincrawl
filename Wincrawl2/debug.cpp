#include "debug.hpp"

#ifdef _MSC_VER
	#include <cstdlib>
	#include <csignal>
	
	#include <windows.h>
	#include <debugapi.h>

	/// Isolate windows headers in this file, so they don't fight with our own Color declarations.
	void Debug::printToConsole(const char* str) { OutputDebugStringA(str); }
#endif