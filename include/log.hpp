// #if HAS_SPDLOG
// #  include <spdlog/spdlog.h>
// #else
// #  include <iostream>
// #  define SPDLOG_INFO(x) std::cout << (x) << "\n";
// #endif

#include <spdlog/spdlog.h>

#ifndef TO_STRING_IMPL
#  define TO_STRING_IMPL(x) #x
#endif

#ifndef TO_STRING
#  define TO_STRING(x) TO_STRING_IMPL(x)
#endif

#ifndef CAT_IMPL
#  define CAT_IMPL(a, b) a##b
#endif

#ifndef CAT
#  define CAT(a, b) CAT_IMPL(a, b)
#endif

#ifndef DBG
#  define DBG(x) "{0}: {1}", TO_STRING(x), (x)
#endif
// clang-format on
