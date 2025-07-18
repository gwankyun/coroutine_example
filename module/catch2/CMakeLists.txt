﻿add_library(catch2_module)
target_sources(catch2_module
  PRIVATE
    catch2/macro.h
    catch2/catch_test_macros.hpp
  PRIVATE FILE_SET CXX_MODULES FILES
    catch2.ixx
    catch2.base.ixx
    catch2.session.ixx
    catch2.modern.ixx
    catch2.compat.ixx
)
target_include_directories(catch2_module INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(catch2_module PUBLIC Catch2::Catch2)
target_compile_features(catch2_module PRIVATE cxx_std_23)
add_library(Catch2::module ALIAS catch2_module)

option(USE_CATCH2_MODULE "使用catch2模塊" ON)

add_library(catch2_module_interface INTERFACE)
target_include_directories(catch2_module_interface
  INTERFACE
    $<$<BOOL:${USE_CATCH2_MODULE}>:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_compile_definitions(catch2_module_interface
  INTERFACE
    USE_CATCH2_MODULE=$<BOOL:${USE_CATCH2_MODULE}>
)
target_link_libraries(catch2_module_interface
  INTERFACE
    $<$<BOOL:${USE_CATCH2_MODULE}>:Catch2::module>
    Catch2::Catch2
)
add_library(Catch2::module::option ALIAS catch2_module_interface)
