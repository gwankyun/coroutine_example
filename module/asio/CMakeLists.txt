add_library(asio_module)
target_sources(asio_module
  PRIVATE FILE_SET CXX_MODULES FILES
    asio.ixx
)
target_link_libraries(asio_module PUBLIC asio::asio)
target_compile_features(asio_module PRIVATE cxx_std_23)
add_library(asio::module ALIAS asio_module)
