#pragma once

#include <functional>
#include <vector>

class Triggers {
	typedef bool isPartialCommand;
	
	struct Trigger {
		const char* seq;
		const std::function<void()> callback;
	};
	
	std::vector<Trigger> triggers{};
	
	
public:
	//If you're feeding a character stream buffer to run(...), isPartialCommand can be
	//used to decide to wait for more characters to be added to the buffer.
	const isPartialCommand run(const char* cmd);
	
	void add(const char* seq, const std::function<void()> callback);
	
	Triggers() {};
	Triggers(std::vector<Trigger> triggers_) : triggers(triggers_) {};
};