#pragma once

#include <functional>

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
	const isPartialCommand run(const char* cmd) {
		const char* command = reinterpret_cast<const char*>(cmd); //Cast for str* functions - it's safe, and we only care about byte-level compatability in this case.
		
		int commandMatchesTriggers{ false };
		for (auto trigger : this->triggers) {
			const char* triggerName = reinterpret_cast<const char*>(trigger.seq);
			
			//Command should be fully equal to run the cb - the full match is needed to absorb all of an escape sequence which could be unique by the second character.
			if (!strcmp(triggerName, command)) {
				std::cout << "\n"; //done input, advance to new line - this probably shouldn't be here, we should return the trigger that matched instead.
				trigger.callback();
				return false;
			}
			
			//Otherwise, mark if our command was equal to part of a trigger. It could be completed in the future.
			commandMatchesTriggers |=
				!strncmp(triggerName, command, strlen(command));
		}
		return commandMatchesTriggers;
	}
	
	void add(const char* seq, const std::function<void()> callback) {
		this->triggers.emplace_back(seq, callback);
	}
};