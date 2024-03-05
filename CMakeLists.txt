# cmake最低版本
cmake_minimum_required(VERSION 3.28)

# 項目名
set(project coroutine_example)
project(${project})

include(CMakePrintHelpers)

# 輸出目錄
set(source_dir ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${source_dir}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${source_dir}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${source_dir}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${source_dir}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${source_dir}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${source_dir}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(common)

cmake_print_variables(CMAKE_SYSTEM)
cmake_print_variables(CMAKE_SYSTEM_NAME)
cmake_print_variables(CMAKE_SYSTEM_VERSION)
cmake_print_variables(CMAKE_SYSTEM_PROCESSOR)

set(msvc_definitions)

if(WIN32)
  get_WIN32_WINNT(_WIN32_WINNT)
  cmake_print_variables(_WIN32_WINNT)
  list(APPEND msvc_definitions "_WIN32_WINNT=${_WIN32_WINNT}")
endif()

list(APPEND msvc_definitions "_CRT_SECURE_NOWARNINGS" "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING")

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/W4;/MP;/Zc:__cplusplus;/experimental:module;>")

add_compile_definitions("$<$<CXX_COMPILER_ID:MSVC>:${msvc_definitions}>")

find_package(Boost REQUIRED COMPONENTS context) # 必需
find_package(spdlog CONFIG) # 可選

add_library(common_required INTERFACE)
set(common_required common_required)
target_sources(${common_required} INTERFACE
  include/coro.hpp
)
target_include_directories(${common_required} INTERFACE include)
target_compile_features(${common_required} INTERFACE cxx_std_20)
target_link_libraries(${common_required} INTERFACE
  Boost::headers
  $<TARGET_NAME_IF_EXISTS:spdlog::spdlog>
)
target_compile_definitions(${common_required} INTERFACE
  HAS_SPDLOG=$<TARGET_EXISTS:spdlog::spdlog>
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
      src/server.cppm
)
target_link_libraries(${server} PRIVATE common_required)

# 客戶端
set(client client)
add_executable(${client})
target_sources(${client}
  PRIVATE
    src/client.cpp
  PRIVATE
  FILE_SET CXX_MODULES
    FILES
      src/client.cppm
)
target_link_libraries(${client} PRIVATE common_required Boost::context)

set(main main)
add_executable(${main})
target_sources(${main} PRIVATE
  src/main.cpp
)
target_link_libraries(${main} PRIVATE common_required)

set(coro_std coro_std)
add_executable(${coro_std})
target_sources(${coro_std} PRIVATE
  src/coro_std.cpp
)
target_link_libraries(${coro_std} PRIVATE common_required)

set(boost_coro2 boost_coro2)
add_executable(${boost_coro2})
target_sources(${boost_coro2} PRIVATE
  src/coro2.cppm src/coro2.cpp
)
target_link_libraries(${boost_coro2} PRIVATE common_required Boost::context)
