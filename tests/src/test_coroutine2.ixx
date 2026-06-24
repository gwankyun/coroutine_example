module;

#include <shared_api.h>

export module test.coroutine2;

import std;

export SHARED_API std::unordered_map<int, std::string> test_coroutine2();
