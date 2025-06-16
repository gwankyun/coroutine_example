add_library(boost_scope_module)
target_sources(boost_scope_module
  PRIVATE
    boost.scope/macro.h
  PRIVATE FILE_SET CXX_MODULES FILES
    boost.scope.ixx)
target_include_directories(boost_scope_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(boost_scope_module PUBLIC Boost::scope)
target_compile_features(boost_scope_module PRIVATE cxx_std_23)
add_library(Boost::scope::module ALIAS boost_scope_module)
