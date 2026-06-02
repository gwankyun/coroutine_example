set(CMAKE_FOLDER tests)


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
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_callback.ixx # 模塊接口單元
  PRIVATE
    src/test_callback.cpp # 模塊實現單元
)
target_link_libraries(
  test_callback
  PRIVATE #
    test_common
)

add_library(test_switch)
target_sources(
  test_switch
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_switch.ixx # 模塊接口單元
  PRIVATE
    src/test_switch.cpp # 模塊實現單元
)
target_link_libraries(
  test_switch
  PRIVATE #
    test_common
)

add_library(test_coroutine2)
target_sources(
  test_coroutine2
  PUBLIC # 對外接口單元需要公開
    FILE_SET CXX_MODULES
      BASE_DIRS src # 模塊根目錄，讓編譯器找到模塊接口
      FILES src/test_coroutine2.ixx # 模塊接口單元
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

#[[
add_executable(tests)
target_sources(tests PRIVATE tests/src/test.cpp)
target_link_libraries(
  tests
  PRIVATE #
    Boost::asio
    Catch2::Catch2
    spdlog::spdlog
    Boost::scope
    test_callback
    test_switch
    test_coroutine2
)
target_compile_features(tests PRIVATE cxx_std_23)

enable_testing()
include(Catch)
catch_discover_tests(tests)
set_tests_properties(${CTEST_TESTS} PROPERTIES ENVIRONMENT "SPDLOG_LEVEL=OFF")
#]]
