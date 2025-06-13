# cmake最低版本
cmake_minimum_required(VERSION 3.28...4.0)

# 項目名
set(project coroutine_example)
project(${project})

include(CMakePrintHelpers) # cmake_print_variables

# 輸出目錄
set(source_dir ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${source_dir}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${source_dir}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${source_dir}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${source_dir}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${source_dir}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${source_dir}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(common)

cmake_print_variables(CMAKE_SYSTEM)
cmake_print_variables(CMAKE_SYSTEM_NAME)
cmake_print_variables(CMAKE_SYSTEM_VERSION)
cmake_print_variables(CMAKE_SYSTEM_PROCESSOR)

cmake_print_variables(CMAKE_GENERATOR)
cmake_print_variables(CMAKE_GENERATOR_INSTANCE)

set(msvc_definitions)

if(WIN32)
  get_WIN32_WINNT(_WIN32_WINNT)
  cmake_print_variables(_WIN32_WINNT)

  # Please define _WIN32_WINNT or _WIN32_WINDOWS appropriately.
  list(APPEND msvc_definitions "_WIN32_WINNT=${_WIN32_WINNT}")
endif()

# warning C4996
option(disable_C4996 "" ON)

if(disable_C4996)
  list(APPEND msvc_definitions "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING")
endif()

add_compile_definitions("$<$<CXX_COMPILER_ID:MSVC>:${msvc_definitions}>")

set(msvc_options)
list(APPEND msvc_options "/W4" "/MP" "/Zc:__cplusplus" "/experimental:module")

# 支持源碼UTF-8
list(APPEND msvc_options "/utf-8")

# warning LNK4099
option(disable_LNK4099 "" ON)

if(disable_LNK4099)
  list(APPEND msvc_options "/Z7")
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>")

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# find_package(Boost REQUIRED COMPONENTS context fiber) # 必需
# check_package(Boost)
find_package(boost_headers CONFIG REQUIRED)
check_package(boost_headers)
find_package(boost_context CONFIG REQUIRED)
check_package(boost_context)
find_package(boost_coroutine2 CONFIG REQUIRED)
check_package(boost_coroutine2)
find_package(boost_fiber CONFIG REQUIRED)
check_package(boost_fiber)
find_package(boost_scope REQUIRED CONFIG)
check_package(boost_scope)
find_package(boost_cobalt REQUIRED CONFIG)
check_package(boost_cobalt)
find_package(boost_asio CONFIG REQUIRED)
check_package(boost_asio)

find_package(asio CONFIG REQUIRED)
check_package(asio)

find_package(spdlog CONFIG REQUIRED)
check_package(spdlog)

find_package(Catch2 CONFIG REQUIRED)
check_package(Catch2)

option(use_cxx_23 "" ON)
set(cxx_std cxx_std_20)

if(use_cxx_23)
  set(cxx_std cxx_std_23)
endif()

find_package(nlohmann_json CONFIG REQUIRED)
check_package(nlohmann_json)

set(common_required common_required)
add_library(${common_required} INTERFACE)
target_include_directories(${common_required} INTERFACE include)
target_compile_features(${common_required} INTERFACE ${cxx_std})
target_link_libraries(${common_required} INTERFACE
  $<TARGET_NAME_IF_EXISTS:spdlog::spdlog>
)
target_compile_definitions(${common_required} INTERFACE
  HAS_SPDLOG=$<TARGET_EXISTS:spdlog::spdlog>
)

set(coro2_required coro2_required)
add_library(${coro2_required} INTERFACE)
target_link_libraries(${coro2_required} INTERFACE
  Boost::headers
  Boost::context
)

set(asio_required asio_required)
add_library(${asio_required} INTERFACE)
target_sources(${asio_required} INTERFACE
  include/coro.hpp
)
target_include_directories(${asio_required} INTERFACE include)
target_link_libraries(${asio_required} INTERFACE
  Boost::headers
  asio::asio
  ${common_required}
  ${coro2_required}
)

# 服務端
set(server server)
add_executable(${server})
target_sources(${server} PRIVATE
  PRIVATE
  src/server.cpp
  PRIVATE
  FILE_SET CXX_MODULES
  FILES
  src/server.ixx
)
target_link_libraries(${server} PRIVATE ${asio_required})

add_subdirectory(callback)
add_subdirectory(switch)
add_subdirectory(coroutine2)
add_subdirectory(context)

# 客戶端
set(client client)
add_executable(${client})
target_sources(${client}
  PRIVATE
  src/client.cpp
  PRIVATE
  FILE_SET CXX_MODULES
  FILES
  src/client.ixx
)
target_link_libraries(${client} PRIVATE ${asio_required})

set(main main)
add_executable(${main})
target_sources(${main} PRIVATE
  src/main.cpp
)
target_link_libraries(${main} PRIVATE ${common_required} Catch2::Catch2)

add_subdirectory(module)
add_subdirectory(asio)

set(coro_std coro_std)
add_executable(${coro_std})
target_sources(${coro_std} PRIVATE
  src/coro_std.cpp
)
target_link_libraries(${coro_std} PRIVATE ${common_required})

set(std_coro std_coro)
add_executable(${std_coro})
target_sources(${std_coro} PRIVATE
  src/std_coro.cpp
)
target_link_libraries(${std_coro} PRIVATE ${common_required} asio::asio)
