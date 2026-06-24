module;

#include <shared_api.h>

export module test.cobalt;

import std;

export SHARED_API std::unordered_map<int, std::string> test_cobalt();
