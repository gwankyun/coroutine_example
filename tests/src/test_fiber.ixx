module;

#include <shared_api.h>

export module test.fiber;

import std;

export SHARED_API std::unordered_map<int, std::string> test_fiber();
