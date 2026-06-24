module;

#include <shared_api.h>

export module test.stdexec;

import std;

export SHARED_API std::unordered_map<int, std::string> test_stdexec();
