#include <chrono>
#include <thread>

#ifdef _MSC_VER
#include <windows.h>
#include <conio>

int getInputChar() {
	return _getch()
}

#else
#include <unistd.h>
#include <termios.h>

//From https://stackoverflow.com/a/16361724
int getInputChar(void) {
	char buf = 0;
	struct termios old = {0};
	fflush(stdout);
	if(tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if(tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr ICANON");
	if(read(0, &buf, 1) < 0)
		perror("read()");
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if(tcsetattr(0, TCSADRAIN, &old) < 0)
		perror("tcsetattr ~ICANON");
	//printf("%c\n", buf); //echo
	return buf;
}

#endif
 
void getInputCharAsync(int* value) { //should be std::atomic<shared_ptr<int>>?
	using namespace std::chrono_literals;
	
	while(*value > -2) {
		*value = getInputChar();
		std::this_thread::yield();
		while (*value >= 0) std::this_thread::sleep_for(1ms);
	}
}