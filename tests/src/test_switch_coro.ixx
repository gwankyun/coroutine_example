module;

#include <shared_api.h>

export module test.switch_coro;

import std;

export SHARED_API std::unordered_map<int, std::string> test_switch_coro();
