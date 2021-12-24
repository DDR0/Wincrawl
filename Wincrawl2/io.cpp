#include <chrono>
#include <thread>

#include "io.hpp"


//Each OS we support has different ways to configure their terminal.
#ifdef _MSC_VER

	#include <windows.h>
	#include <debugapi.h>
	#include <conio.h>
	
	bool setUpConsole() {
		//Enable UTF-8 debugging in VS output panel as per https://docs.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-outputdebugstringa.
		DEBUG_EVENT _evt{};
		WaitForDebugEventEx(&_evt, 1);

		//Set the text console itself to UTF-8.
		return SetConsoleCP(CP_UTF8) & SetConsoleOutputCP(CP_UTF8);
	}

	bool tearDownConsole() {
		return true;
	}

	int getInputChar() {
		return _getch();
	};

#else

	#include <unistd.h>
	#include <termios.h>

	struct termios old = { 0 };

	bool setUpConsole() {
		errno = 0;
		if (tcgetattr(0, &old) < 0)
			perror("tcsetattr()");
		old.c_lflag &= ~ICANON;
		old.c_lflag &= ~ECHO;
		old.c_cc[VMIN] = 1;
		old.c_cc[VTIME] = 0;
		if (tcsetattr(0, TCSANOW, &old) < 0)
			perror("tcsetattr ICANON");
		return true;
	}

	bool tearDownConsole() {
		old.c_lflag |= ICANON;
		old.c_lflag |= ECHO;
		if (tcsetattr(0, TCSADRAIN, &old) < 0)
			perror("tcsetattr ~ICANON");
		return true;
	}

	//From https://stackoverflow.com/a/16361724
	int getInputChar(void) {
		fflush(stdout);
		
		char buf = 0;
		if (read(0, &buf, 1) < 0)
			perror("read()");
		
		//printf("%c\n", buf); //echo
		return buf;
	}

#endif 



//Wrap above in a little read loop, to be called as a thread.
void getInputCharAsync(std::atomic<int>& value) { //TODO: Use std::stop_token here?
	using namespace std::chrono_literals;

	while (true) { //-2 is "exit", -1 is "ready for next char".
		int status = value.exchange(getInputChar()); //Blocking.
		if (status == getInputCharAsync::stop) break;

		std::this_thread::yield(); //Yield to our other thread if it's ready now.
		while (value >= 0)         //Otherwise wait 1ms and try again.
			std::this_thread::sleep_for(1ms);
		if (value == getInputCharAsync::stop) break;
	}
}