#include <cstring>
#include <iostream>

#include "triggers.hpp"

//Register a new callback.
void Triggers::add(const char* seq, const std::function<void()> callback) {
	this->triggers.emplace_back(seq, callback);
}

//Run a registered callback. Returns true if the command was a partial match for a trigger, and therefore wasn't consumed.
bool Triggers::run(const char* command) {
	int commandMatchesTriggers{ false };

	for (auto trigger : this->triggers) {
		//Command should be fully equal to run the cb - the full match is needed to absorb all of an escape sequence which could be unique by the second character.
		if (!strcmp(trigger.seq, command)) {
			std::cerr << "\n"; //done input, advance to new line - this probably shouldn't be here, we should return the trigger that matched instead.
			trigger.callback();
			return false;
		}
		
		//Mark if our command so far is a partial match to a trigger. It could be completed in the future as more characters are read in.
		commandMatchesTriggers |= !strncmp(trigger.seq, command, strlen(command));
	}

	return commandMatchesTriggers;
}