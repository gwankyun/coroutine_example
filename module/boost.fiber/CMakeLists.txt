add_library(boost_fiber_module)
target_sources(boost_fiber_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.fiber.ixx)
target_link_libraries(boost_fiber_module PUBLIC Boost::headers Boost::context Boost::fiber)
target_compile_features(boost_fiber_module PRIVATE cxx_std_23)
add_library(Boost::fiber::module ALIAS boost_fiber_module)
