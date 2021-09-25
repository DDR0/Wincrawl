#pragma once

#include <atomic>

bool setUpConsole();
bool tearDownConsole();

int getInputChar(void);

enum getInputCharAsync { next = -1, stop = -2 };
void getInputCharAsync(std::atomic<int>& value);