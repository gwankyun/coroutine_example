set(asio_common asio_common)
add_library(${asio_common} INTERFACE)
target_link_libraries(${asio_common} INTERFACE
  Boost::headers
  spdlog::spdlog
)
target_include_directories(${asio_common} INTERFACE include)

set(context_common context_common)
add_library(${context_common} INTERFACE)
target_link_libraries(${context_common} INTERFACE
  ${asio_common}
  Boost::context
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
