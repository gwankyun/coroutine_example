# cmake最低版本
cmake_minimum_required(VERSION 3.23)

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

if(WIN32)
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

  get_WIN32_WINNT(ver)
  cmake_print_variables(CMAKE_SYSTEM_VERSION)
  cmake_print_variables(ver)
  add_definitions(-D_WIN32_WINNT=${ver})
endif()

find_package(Boost REQUIRED) # 必需
find_package(spdlog CONFIG) # 可選

# 服務端
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

# 客戶端
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
