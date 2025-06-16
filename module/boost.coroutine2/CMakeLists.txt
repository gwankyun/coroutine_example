add_library(boost_coroutine2_module)
target_sources(boost_coroutine2_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.coroutine2.ixx)
target_link_libraries(boost_coroutine2_module PUBLIC Boost::headers Boost::context)
target_compile_features(boost_coroutine2_module PRIVATE cxx_std_23)
add_library(Boost::coroutine2::module ALIAS boost_coroutine2_module)
