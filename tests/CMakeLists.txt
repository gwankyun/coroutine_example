set(CMAKE_FOLDER test)

add_library(test_common INTERFACE)
target_link_libraries(
  test_common
  INTERFACE #
    Boost::asio
    spdlog::spdlog
)
target_compile_features(test_common INTERFACE cxx_std_23)

add_library(test_callback)
target_sources(
  test_callback
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS
        src # 模塊根目錄，讓編譯器找到模塊接口
      FILES
        src/test_callback.ixx # 模塊接口單元
  PRIVATE
    src/test_callback.cpp # 模塊實現單元
)
target_link_libraries(
  test_callback
  PRIVATE #
    test_common
)

add_library(test_switch_coro)
target_sources(
  test_switch_coro
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS
        src # 模塊根目錄，讓編譯器找到模塊接口
      FILES
        src/test_switch_coro.ixx # 模塊接口單元
  PRIVATE
    src/test_switch_coro.cpp # 模塊實現單元
)
target_link_libraries(
  test_switch_coro
  PRIVATE #
    test_common
)

add_library(test_coroutine2)
target_sources(
  test_coroutine2
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS
        src # 模塊根目錄，讓編譯器找到模塊接口
      FILES
        src/test_coroutine2.ixx # 模塊接口單元
  PRIVATE
    src/test_coroutine2.cpp # 模塊實現單元
)
target_link_libraries(
  test_coroutine2
  PRIVATE #
    test_common
    Boost::coroutine2
    Boost::scope
)

add_library(test_context_continuation)
target_sources(
  test_context_continuation
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS
        src # 模塊根目錄，讓編譯器找到模塊接口
      FILES
        src/test_context_continuation.ixx # 模塊接口單元
  PRIVATE
    src/test_context_continuation.cpp # 模塊實現單元
)
target_link_libraries(
  test_context_continuation
  PRIVATE #
    test_common
    Boost::context
    Boost::scope
)

add_library(test_context_fiber)
target_sources(
  test_context_fiber
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS
        src # 模塊根目錄，讓編譯器找到模塊接口
      FILES
        src/test_context_fiber.ixx # 模塊接口單元
  PRIVATE
    src/test_context_fiber.cpp # 模塊實現單元
)
target_link_libraries(
  test_context_fiber
  PRIVATE #
    test_common
    Boost::context
    Boost::scope
)

add_library(test_fiber)
target_sources(
  test_fiber
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS
        src # 模塊根目錄，讓編譯器找到模塊接口
      FILES
        src/test_fiber.ixx # 模塊接口單元
  PRIVATE
    src/test_fiber.cpp # 模塊實現單元
  PRIVATE #
    FILE_SET HEADERS
      BASE_DIRS
        src #
      FILES src/fiber/asio/round_robin.hpp
)
target_link_libraries(
  test_fiber
  PRIVATE #
    test_common
    Boost::fiber
)
