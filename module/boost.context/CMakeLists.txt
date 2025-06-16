add_library(boost_context_module)
target_sources(boost_context_module
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.context.ixx)
target_link_libraries(boost_context_module PUBLIC Boost::headers Boost::context)
target_compile_features(boost_context_module PRIVATE cxx_std_23)
add_library(Boost::context::module ALIAS boost_context_module)
