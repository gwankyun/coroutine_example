set(callcc callcc)

set(callcc_required callcc_required)
add_library(${callcc_required} INTERFACE)
target_sources(${callcc_required}
  INTERFACE
    ${CMAKE_SOURCE_DIR}/include/connection.h
)
target_include_directories(${callcc_required}
  INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(${callcc_required} INTERFACE
  Boost::headers
  Boost::context
  spdlog::spdlog
)
target_compile_features(${callcc_required} INTERFACE cxx_std_23)

set(callcc_example callcc_example)
add_executable(${callcc_example})
target_sources(${callcc_example} PRIVATE
  src/callcc.cpp
)
target_link_libraries(${callcc_example}
  PRIVATE
    ${callcc_required}
)
set_target_properties(${callcc_example} PROPERTIES FOLDER ${callcc})
