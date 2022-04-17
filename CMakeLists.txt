# cmake最低版本
cmake_minimum_required(VERSION 3.23)

# 項目名
set(project coroutine_example)
project(${project})

# 啟用IDE目錄功能
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 輸出目錄
set(source_dir ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${source_dir}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${source_dir}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${source_dir}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${source_dir}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${source_dir}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${source_dir}/lib)

find_package(Boost REQUIRED)
find_package(spdlog CONFIG)

set(server server)
add_executable(${server})
target_sources(${server} PRIVATE
  src/server.cpp
  include/coro.hpp
)
target_include_directories(${server} PRIVATE include)
target_compile_features(${server} PUBLIC cxx_std_20)
target_link_libraries(${server} PRIVATE
  Boost::headers
  $<TARGET_NAME_IF_EXISTS:spdlog::spdlog>
)
target_compile_definitions(${server} PRIVATE
  SPDLOG_EXISTS=$<TARGET_EXISTS:spdlog::spdlog>
)

set(client client)
add_executable(${client})
target_sources(${client} PRIVATE
  src/client.cpp
  include/coro.hpp
)
target_include_directories(${client} PRIVATE include)
target_compile_features(${client} PUBLIC cxx_std_20)
target_link_libraries(${client} PRIVATE
  Boost::headers
  $<TARGET_NAME_IF_EXISTS:spdlog::spdlog>
)
target_compile_definitions(${client} PRIVATE
  SPDLOG_EXISTS=$<TARGET_EXISTS:spdlog::spdlog>
)
