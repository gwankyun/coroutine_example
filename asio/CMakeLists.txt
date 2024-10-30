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

set(context_common context_common)
add_library(${context_common} INTERFACE)
target_link_libraries(${context_common} INTERFACE
  ${asio_common}
  Boost::context
  ${on_exit}
  Boost::scope
)

set(asio_callback asio_callback)
add_executable(${asio_callback})
target_sources(${asio_callback} PRIVATE
  src/asio_callback.cpp
)
target_link_libraries(${asio_callback} PRIVATE
  ${asio_common}
)

set(asio_switch asio_switch)
add_executable(${asio_switch})
target_sources(${asio_switch} PRIVATE
  src/asio_switch.cpp
)
target_link_libraries(${asio_switch} PRIVATE
  ${asio_common}
)

set(asio_coroutine2 asio_coroutine2)
add_executable(${asio_coroutine2})
target_sources(${asio_coroutine2} PRIVATE
  src/asio_coroutine2.cpp
)
target_link_libraries(${asio_coroutine2} PRIVATE
  ${context_common}
)

set(asio_continuatioin asio_continuatioin)
add_executable(${asio_continuatioin})
target_sources(${asio_continuatioin} PRIVATE
  src/asio_continuatioin.cpp
)
target_link_libraries(${asio_continuatioin} PRIVATE
  ${context_common}
)

set(asio_context_fiber asio_context_fiber)
add_executable(${asio_context_fiber})
target_sources(${asio_context_fiber} PRIVATE
  src/asio_context_fiber.cpp
)
target_link_libraries(${asio_context_fiber} PRIVATE
  ${context_common}
)

set(asio_fiber asio_fiber)
add_executable(${asio_fiber})
target_sources(${asio_fiber} PRIVATE
  src/asio_fiber.cpp
)
target_link_libraries(${asio_fiber} PRIVATE
  ${context_common}
  Boost::fiber
)
target_compile_features(${asio_fiber} PRIVATE cxx_std_20)

set(asio_std_coroutine asio_std_coroutine)
add_executable(${asio_std_coroutine})
target_sources(${asio_std_coroutine} PRIVATE
  src/asio_std_coroutine.cpp
)
target_link_libraries(${asio_std_coroutine} PRIVATE
  ${asio_common}
  ${asio_lite}
)
target_compile_features(${asio_std_coroutine} PRIVATE cxx_std_20)

set(asio_strand asio_strand)
add_executable(${asio_strand})
target_sources(${asio_strand} PRIVATE
  src/asio_strand.cpp
)
target_link_libraries(${asio_strand} PRIVATE
  ${asio_common}
  ${asio_lite}
)
target_compile_features(${asio_strand} PRIVATE cxx_std_20)
