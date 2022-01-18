//Escape sequences.
#pragma once

struct seq {
	static inline const char* bold         { "[1m"   };
	static inline const char* boldOff      { "[22m"  };
	static inline const char* clear        { "c"     };
	static inline const char* moveToOrigin { "[0;0H" };
	static inline const char* reset        { "[0m"   };
	static inline const char* underline    { "[4m"   };
	static inline const char* underlineOff { "[24m"  };
	static inline const char* hideCursor   { "[?25l" };
	static inline const char* showCursor   { "[?25h" };
};