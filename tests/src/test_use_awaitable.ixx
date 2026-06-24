module;

#include <shared_api.h>

export module test.use_awaitable;

import std;

export SHARED_API std::unordered_map<int, std::string> test_use_awaitable();
