set(asio asio)

set(CMAKE_FOLDER ${asio})

set(on_exit on_exit)
add_library(${on_exit})
target_sources(${on_exit}
  PUBLIC
  include/on_exit.h
  PRIVATE
  include/on_exit.hpp
  src/on_exit.cpp
)

set(time_count time_count)
add_library(${time_count})
target_sources(${time_count}
  PUBLIC
  include/time_count.h
  PRIVATE
  include/time_count.hpp
  src/time_count.cpp
)

set(asio_lite asio_lite)
add_library(${asio_lite})
target_sources(${asio_lite}
  PUBLIC
  include/asio_lite.h
  PRIVATE
  src/asio_lite.cpp
)

set(asio_common asio_common)
add_library(${asio_common} INTERFACE)
target_link_libraries(${asio_common} INTERFACE
  Boost::headers
  spdlog::spdlog
  ${time_count}
  Catch2::Catch2
  asio::asio
)
target_include_directories(${asio_common} INTERFACE include ${CMAKE_SOURCE_DIR}/include)
target_compile_features(${asio_common} INTERFACE cxx_std_23)

set(context_common context_common)
add_library(${context_common} INTERFACE)
target_link_libraries(${context_common} INTERFACE
  ${asio_common}
  Boost::context
  ${on_exit}
  Boost::scope
)

option(USE_STD_MODULE "使用標準庫模塊" ON)
option(USE_THIRD_MODULE "使用第三方庫模塊" ON)
option(USE_ASIO_STANDALONE "使用非boost版asio" OFF)
option(USE_ASIO_MODULE "使用asio模塊" OFF)
# option(USE_BOOST_ASIO_MODULE "使用boost.asio模塊" ON)

option(USE_BOOST_FIBER_MODULE "使用boost.fiber模塊" OFF)
option(USE_CATCH2_MODULE "使用catch2模塊" ON)
# option(USE_SPDLOG_MODULE "使用spdlog模塊" ON)

add_library(catch2_interface INTERFACE)
target_compile_definitions(catch2_interface
  INTERFACE
    USE_CATCH2_MODULE=$<BOOL:${USE_CATCH2_MODULE}>
)
target_link_libraries(catch2_interface
  INTERFACE
    $<IF:$<BOOL:${USE_CATCH2_MODULE}>,Catch2::module,Catch2::Catch2>
)

add_library(spdlog_interface INTERFACE)
target_compile_definitions(spdlog_interface
  INTERFACE
    USE_SPDLOG_MODULE=$<BOOL:${USE_SPDLOG_MODULE}>
)
target_link_libraries(spdlog_interface
  INTERFACE
    $<IF:$<BOOL:${USE_SPDLOG_MODULE}>,spdlog::module,spdlog::spdlog>
)

add_library(asio_interface INTERFACE)
target_compile_definitions(asio_interface
  INTERFACE
    # USE_ASIO_STANDALONE=$<BOOL:${USE_ASIO_STANDALONE}>
    USE_BOOST_ASIO_MODULE=$<BOOL:${USE_BOOST_ASIO_MODULE}>
)
target_link_libraries(asio_interface
  INTERFACE
    # $<IF:$<BOOL:${USE_ASIO_STANDALONE}>,asio::asio,Boost::asio>
    Boost::asio Boost::headers
    $<$<BOOL:${USE_BOOST_ASIO_MODULE}>:Boost::asio::module>
)

add_library(third_module INTERFACE)
target_compile_definitions(third_module
  INTERFACE
    USE_STD_MODULE=$<BOOL:${USE_STD_MODULE}>
    USE_THIRD_MODULE=$<BOOL:${USE_THIRD_MODULE}>
)
target_link_libraries(third_module
  INTERFACE
    Catch2::module
    spdlog::module
)

add_library(cxx_std_interface INTERFACE)
target_compile_features(cxx_std_interface INTERFACE cxx_std_23)

add_executable(asio_callback)
target_sources(asio_callback
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_callback.ixx
)
target_link_libraries(asio_callback
  PRIVATE
    Catch2::module::option
    # spdlog_interface
    spdlog::module::option
    asio_interface
    cxx_std_interface
)

add_executable(asio_switch)
target_sources(asio_switch PRIVATE
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_switch.ixx
)
target_link_libraries(asio_switch
  PRIVATE
    Catch2::module::option
    # spdlog_interface
    spdlog::module::option
    asio_interface
    cxx_std_interface
)


add_library(boost_fiber_module_interface INTERFACE)
target_compile_definitions(boost_fiber_module_interface
  INTERFACE
    USE_BOOST_FIBER_MODULE=$<BOOL:${USE_BOOST_FIBER_MODULE}>
)
target_link_libraries(boost_fiber_module_interface
  INTERFACE
    $<IF:$<BOOL:${USE_BOOST_FIBER_MODULE}>,Boost::fiber::module,Boost::fiber>
)
add_library(fiber_interface ALIAS boost_fiber_module_interface)


add_executable(asio_coroutine2)
target_sources(asio_coroutine2
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_coroutine2.ixx
)
target_link_libraries(asio_coroutine2
  PRIVATE
    Catch2::module::option
    # spdlog_interface
    spdlog::module::option
    Boost::context::module::option
    asio_interface
    Boost::scope::module::option
    Boost::coroutine2::module::option
    cxx_std_interface
)

add_executable(asio_continuatioin)
target_sources(asio_continuatioin PRIVATE
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_continuatioin.ixx
)
target_link_libraries(asio_continuatioin
  PRIVATE
    Catch2::module::option
    # spdlog_interface
    spdlog::module::option
    asio_interface
    Boost::scope::module::option
    Boost::context::module::option
    cxx_std_interface
)

add_executable(asio_context_fiber)
target_sources(asio_context_fiber PRIVATE
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_context_fiber.ixx
)
target_link_libraries(asio_context_fiber
  PRIVATE
    Catch2::module::option
    # spdlog_interface
    spdlog::module::option
    asio_interface
    Boost::scope::module::option
    Boost::context::module::option
    cxx_std_interface
)

add_executable(asio_fiber)
target_sources(asio_fiber PRIVATE
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_fiber.ixx
)
target_link_libraries(asio_fiber PRIVATE
  Catch2::module::option
  # spdlog_interface
  spdlog::module::option
  asio_interface
  Boost::scope::module::option
  fiber_interface
  cxx_std_interface
)
target_compile_features(asio_fiber PRIVATE cxx_std_23)

add_executable(asio_std_coroutine)
target_sources(asio_std_coroutine PRIVATE
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_std_coroutine.ixx
)
target_link_libraries(asio_std_coroutine PRIVATE
  ${asio_common}
  ${asio_lite}
  third_module
)
target_compile_features(asio_std_coroutine PRIVATE cxx_std_23)

add_executable(asio_strand)
target_sources(asio_strand PRIVATE
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_strand.ixx
)
target_link_libraries(asio_strand PRIVATE
  ${asio_common}
  ${asio_lite}
  third_module
)
target_compile_features(asio_strand PRIVATE cxx_std_23)

add_executable(asio_cobalt)
target_sources(asio_cobalt PRIVATE
  PRIVATE FILE_SET CXX_MODULES FILES
    src/asio_cobalt.ixx
)
target_link_libraries(asio_cobalt PRIVATE
  Boost::cobalt
  spdlog::spdlog
  Boost::headers
  asio::asio
  third_module
)
target_compile_features(asio_cobalt PRIVATE cxx_std_23)
