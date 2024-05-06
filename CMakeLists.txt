# cmake最低版本
cmake_minimum_required(VERSION 3.29.2)

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

# warning LNK4099
option(disable_LNK4099 "" ON)
if(disable_LNK4099)
  list(APPEND msvc_options "/Z7")
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>")

if(${CMAKE_VERSION} VERSION_LESS_EQUAL "3.29.2")
  set(Boost_NO_WARN_NEW_VERSIONS ON)
  set(BOOST_ROOT "$ENV{ProgramFiles}/boost_1_85_0")
endif()
find_package(Boost REQUIRED COMPONENTS context) # 必需
check_package(Boost)

find_package(spdlog CONFIG) # 可選
check_package(spdlog)

option(use_cxx_23 "" ON)
set(cxx_std cxx_std_20)
if(use_cxx_23)
  set(cxx_std cxx_std_23)
endif()

find_package(nlohmann_json REQUIRED)
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
target_link_libraries(${asio_required} INTERFACE
  Boost::headers
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
target_link_libraries(${main} PRIVATE ${common_required})

set(coro_std coro_std)
add_executable(${coro_std})
target_sources(${coro_std} PRIVATE
  src/coro_std.cpp
)
target_link_libraries(${coro_std} PRIVATE ${common_required})

# set(asio asio)
# add_executable(${asio})
# target_sources(${asio} PRIVATE
#   src/asio.cpp
# )
# target_link_libraries(${asio} PRIVATE
#   Boost::headers
#   spdlog::spdlog
# )

add_subdirectory(asio)

# set(boost_coro2 boost_coro2)
# add_executable(${boost_coro2})
# target_sources(${boost_coro2} PRIVATE
#   src/coro2.ixx src/coro2.cpp
# )
# target_link_libraries(${boost_coro2} PRIVATE
#   ${common_required}
#   ${coro2_required}
# )
