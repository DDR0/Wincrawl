//Escape sequences.
#pragma once

struct seq {
	inline static const char* clear { "c" };
	inline static const char* reset { "[0m" };
	inline static const char* bold  { "[1m" };
};