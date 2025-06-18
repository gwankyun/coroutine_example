#pragma once

#if !USE_CATCH2_MODULE
#  include <catch2/../catch2/catch_session.hpp>
#else
#  include <catch2/macro.h>
#endif
// clang-format off
#include <catch2/catch_test_macros.hpp>
// clang-format on
