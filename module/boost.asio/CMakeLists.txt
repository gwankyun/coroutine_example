add_library(boost_asio_module)
target_sources(boost_asio_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.asio.ixx
)
target_link_libraries(boost_asio_module PUBLIC Boost::headers Boost::asio)
target_compile_features(boost_asio_module PRIVATE cxx_std_23)
add_library(Boost::asio::module ALIAS boost_asio_module)
