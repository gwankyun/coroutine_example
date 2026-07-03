set(CMAKE_FOLDER test)

add_library(test_common INTERFACE)
target_link_libraries(
  test_common
  INTERFACE #
    Boost::asio
    spdlog::spdlog
)
target_compile_features(test_common INTERFACE cxx_std_23)

add_library(shared_api INTERFACE)
target_sources(
  shared_api
  INTERFACE #
    FILE_SET HEADERS BASE_DIRS src FILES src/shared_api.h
)
target_compile_definitions(shared_api INTERFACE SHARED_EXPORT)

set(test_lib_type STATIC)

# === 模块库（提供測試函數的實現） ===

add_library(test_callback )
target_sources(
  test_callback
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_callback.ixx
  PRIVATE src/test_callback.cpp
)
target_link_libraries(
  test_callback
  PRIVATE #
    test_common
    shared_api
)

add_library(test_switch_coro )
target_sources(
  test_switch_coro
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_switch_coro.ixx
  PRIVATE src/test_switch_coro.cpp
)
target_link_libraries(
  test_switch_coro
  PRIVATE #
    test_common
    shared_api
)

add_library(io_scheduler)
target_sources(
  io_scheduler
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/io_scheduler.ixx
)
target_link_libraries(
  io_scheduler
  PRIVATE #
    test_common
    shared_api
)

add_library(test_coroutine2 )
target_sources(
  test_coroutine2
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_coroutine2.ixx
  PRIVATE src/test_coroutine2.cpp
)
target_link_libraries(
  test_coroutine2
  PRIVATE #
    test_common
    Boost::coroutine2
    Boost::scope
    io_scheduler
    shared_api
)

add_library(test_context_continuation )
target_sources(
  test_context_continuation
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_context_continuation.ixx
  PRIVATE src/test_context_continuation.cpp
)
target_link_libraries(
  test_context_continuation
  PRIVATE #
    test_common
    Boost::context
    Boost::scope
    io_scheduler
    shared_api
)

add_library(test_context_fiber )
target_sources(
  test_context_fiber
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_context_fiber.ixx
  PRIVATE src/test_context_fiber.cpp
)
target_link_libraries(
  test_context_fiber
  PRIVATE #
    test_common
    Boost::context
    Boost::scope
    io_scheduler
    shared_api
)

add_library(test_fiber )
target_sources(
  test_fiber
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_fiber.ixx
  PRIVATE src/test_fiber.cpp
  PRIVATE #
    FILE_SET HEADERS BASE_DIRS src
)
target_link_libraries(
  test_fiber
  PRIVATE #
    test_common
    Boost::fiber
    shared_api
)

add_library(test_std_coroutine )
target_sources(
  test_std_coroutine
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES
        src/test_std_coroutine.ixx
        src/stdcoro.ixx
  PRIVATE src/test_std_coroutine.cpp
)
target_link_libraries(
  test_std_coroutine
  PRIVATE #
    test_common
    shared_api
)

add_library(test_use_awaitable )
target_sources(
  test_use_awaitable
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_use_awaitable.ixx
  PRIVATE src/test_use_awaitable.cpp
)
target_link_libraries(
  test_use_awaitable
  PRIVATE #
    test_common
    shared_api
)

add_library(test_cobalt )
target_sources(
  test_cobalt
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_cobalt.ixx
  PRIVATE src/test_cobalt.cpp
)
target_link_libraries(
  test_cobalt
  PRIVATE #
    test_common
    Boost::cobalt
    shared_api
)

add_library(test_stdexec )
target_sources(
  test_stdexec
  PUBLIC
    FILE_SET CXX_MODULES
      BASE_DIRS src
      FILES src/test_stdexec.ixx
  PRIVATE src/test_stdexec.cpp
)
target_link_libraries(
  test_stdexec
  PRIVATE #
    test_common
    STDEXEC::stdexec
    STDEXEC::asioexec
    STDEXEC::parallel_scheduler
    shared_api
)

# === 獨立測試 exe ===

function(add_test_exe TEST_NAME MODULE_LIB)
  add_executable(run_${TEST_NAME})
  target_sources(run_${TEST_NAME} PRIVATE src/main/test_${TEST_NAME}_main.cpp)
  target_link_libraries(
    run_${TEST_NAME}
    PRIVATE #
      Catch2::Catch2
      spdlog::spdlog
      ${MODULE_LIB}
  )
  target_compile_features(run_${TEST_NAME} PRIVATE cxx_std_23)
  add_test(NAME ${TEST_NAME} COMMAND run_${TEST_NAME})
  set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "SPDLOG_LEVEL=OFF")
endfunction()

add_test_exe(callback test_callback)
add_test_exe(switch_coro test_switch_coro)
add_test_exe(coroutine2 test_coroutine2)
add_test_exe(context_continuation test_context_continuation)
add_test_exe(context_fiber test_context_fiber)
add_test_exe(fiber test_fiber)
add_test_exe(std_coroutine test_std_coroutine)
add_test_exe(use_awaitable test_use_awaitable)
add_test_exe(cobalt test_cobalt)
add_test_exe(stdexec test_stdexec)
