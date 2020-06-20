#pragma once
#include <functional>
#include "vm.h"

void RunScreen(const char *classname, VMValue *params, int numparams, std::function<void(bool aborted)> completion);
