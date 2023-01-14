# cmake最低版本
cmake_minimum_required(VERSION 3.25)

# 項目名
set(project coroutine_example)
project(${project})

include(CMakePrintHelpers)

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

function(get_WIN32_WINNT version)
  if(CMAKE_SYSTEM_VERSION)
    set(ver ${CMAKE_SYSTEM_VERSION})
    string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
    string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
    # Check for Windows 10, b/c we'll need to convert to hex 'A'.
    if("${verMajor}" MATCHES "10")
      set(verMajor "A")
      string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
    endif()
    # Remove all remaining '.' characters.
    string(REPLACE "." "" ver ${ver})
    # Prepend each digit with a zero.
    string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
    set(${version} "0x${ver}" PARENT_SCOPE)
  endif()
endfunction()

cmake_print_variables(CMAKE_SYSTEM)
cmake_print_variables(CMAKE_SYSTEM_NAME)
cmake_print_variables(CMAKE_SYSTEM_VERSION)
cmake_print_variables(CMAKE_SYSTEM_PROCESSOR)
if(WIN32)
  get_WIN32_WINNT(_WIN32_WINNT)
  cmake_print_variables(_WIN32_WINNT)
  add_compile_definitions(_WIN32_WINNT=${_WIN32_WINNT})
endif()

set(Boost_ROOT "c:/Program Files/boost_1_81_0")
find_package(Boost REQUIRED) # 必需
find_package(spdlog CONFIG) # 可選

add_library(common_required INTERFACE)
target_include_directories(common_required INTERFACE include)
target_compile_features(common_required INTERFACE cxx_std_20)
target_link_libraries(common_required INTERFACE
  Boost::headers
  $<TARGET_NAME_IF_EXISTS:spdlog::spdlog>
)
target_compile_definitions(common_required INTERFACE
  HAS_SPDLOG=$<TARGET_EXISTS:spdlog::spdlog>
)

# 服務端
set(server server)
add_executable(${server})
target_sources(${server} PRIVATE
  src/server.cpp
  include/coro.hpp
)
target_link_libraries(${server} PRIVATE common_required)

# 客戶端
set(client client)
add_executable(${client})
target_sources(${client} PRIVATE
  src/client.cpp
  include/coro.hpp
)
target_link_libraries(${client} PRIVATE common_required)
