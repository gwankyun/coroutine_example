module;

#include <shared_api.h>

export module test.std_coroutine;

import std;

export SHARED_API std::unordered_map<int, std::string> test_std_coroutine();
