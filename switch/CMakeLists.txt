cmake_print_variables(CMAKE_SOURCE_DIR)
cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)

set(switch_required switch_required)
add_library(${switch_required} INTERFACE)
target_sources(${switch_required} INTERFACE
  ${CMAKE_SOURCE_DIR}/include/log.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/common.h
  # ${CMAKE_CURRENT_SOURCE_DIR}/include/data.hpp
  # ${CMAKE_CURRENT_SOURCE_DIR}/src/common.cpp
)
target_compile_features(${switch_required} INTERFACE cxx_std_23)
target_include_directories(${switch_required}
  INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${switch_required} INTERFACE
  Boost::headers
  spdlog::spdlog
)

set(server_switch server_switch)
add_executable(${server_switch})
target_sources(${server_switch}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server/switch.cpp
)
target_link_libraries(${server_switch}
  PRIVATE
    ${switch_required}
)

set(client_switch client_switch)
add_executable(${client_switch})
target_sources(${client_switch}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client/switch.cpp
)
target_link_libraries(${client_switch}
  PRIVATE
    ${switch_required}
)
