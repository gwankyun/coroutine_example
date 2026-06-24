module;

#include <shared_api.h>

export module test.context.continuation;

import std;

export SHARED_API std::unordered_map<int, std::string> test_context_continuation();
