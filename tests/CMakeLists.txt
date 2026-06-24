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

add_library(test_callback SHARED)
target_sources(
  test_callback
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_callback.ixx # 模塊接口單元
  PRIVATE src/test_callback.cpp # 模塊實現單元
)
target_link_libraries(
  test_callback
  PRIVATE #
    test_common
    shared_api
)

add_library(test_switch_coro SHARED)
target_sources(
  test_switch_coro
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_switch_coro.ixx # 模塊接口單元
  PRIVATE src/test_switch_coro.cpp # 模塊實現單元
)
target_link_libraries(
  test_switch_coro
  PRIVATE #
    test_common
    shared_api
)

add_library(io_scheduler SHARED)
target_sources(
  io_scheduler
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/io_scheduler.ixx # 模塊接口單元
)
target_link_libraries(
  io_scheduler
  PRIVATE #
    test_common
    shared_api
)

add_library(test_coroutine2 SHARED)
target_sources(
  test_coroutine2
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_coroutine2.ixx # 模塊接口單元
  PRIVATE src/test_coroutine2.cpp # 模塊實現單元
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

add_library(test_context_continuation SHARED)
target_sources(
  test_context_continuation
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_context_continuation.ixx # 模塊接口單元
  PRIVATE src/test_context_continuation.cpp # 模塊實現單元
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

add_library(test_context_fiber SHARED)
target_sources(
  test_context_fiber
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_context_fiber.ixx # 模塊接口單元
  PRIVATE src/test_context_fiber.cpp # 模塊實現單元
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

add_library(test_fiber SHARED)
target_sources(
  test_fiber
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_fiber.ixx # 模塊接口單元
  PRIVATE src/test_fiber.cpp # 模塊實現單元
  PRIVATE #
    FILE_SET HEADERS BASE_DIRS src # FILES src/fiber/asio/round_robin.hpp
)
target_link_libraries(
  test_fiber
  PRIVATE #
    test_common
    Boost::fiber
    shared_api
)

add_library(test_std_coroutine SHARED)
target_sources(
  test_std_coroutine
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES
        src/test_std_coroutine.ixx # 模塊接口單元
        src/stdcoro.ixx
  PRIVATE src/test_std_coroutine.cpp # 模塊實現單元
)
target_link_libraries(
  test_std_coroutine
  PRIVATE #
    test_common
    shared_api
)

add_library(test_use_awaitable SHARED)
target_sources(
  test_use_awaitable
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_use_awaitable.ixx # 模塊接口單元
  PRIVATE src/test_use_awaitable.cpp # 模塊實現單元
)
target_link_libraries(
  test_use_awaitable
  PRIVATE #
    test_common
    shared_api
)

add_library(test_cobalt SHARED)
target_sources(
  test_cobalt
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_cobalt.ixx # 模塊接口單元
  PRIVATE src/test_cobalt.cpp # 模塊實現單元
)
target_link_libraries(
  test_cobalt
  PRIVATE #
    test_common
    Boost::cobalt
    shared_api
)

add_library(test_stdexec SHARED)
target_sources(
  test_stdexec
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_stdexec.ixx # 模塊接口單元
  PRIVATE src/test_stdexec.cpp # 模塊實現單元
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
