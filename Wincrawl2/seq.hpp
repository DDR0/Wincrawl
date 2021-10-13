//Escape sequences.
#pragma once

struct seq {
	inline static const char* bold         { "[1m"   };
	inline static const char* boldOff      { "[22m"  };
	inline static const char* clear        { "c"     };
	inline static const char* moveToOrigin { "[0;0H" };
	inline static const char* reset        { "[0m"   };
	inline static const char* underline    { "[4m"   };
	inline static const char* underlineOff { "[24m"  };
};