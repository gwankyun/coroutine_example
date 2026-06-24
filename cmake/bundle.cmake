include(BundleUtilities)

if(NOT DEFINED libs)
  set(libs "")
endif()

if(NOT DEFINED ignoreItems)
  set(ignoreItems "")
endif()

function(gp_item_default_embedded_path_override item default_path)
  set(path "@executable_path" PARENT_SCOPE)
endfunction()

foreach(app IN LISTS apps)
  fixup_bundle(
    "${app}"
    "${libs}"
    "${dirs}"
    IGNORE_ITEM
    "${ignoreItems}"
  )
endforeach()
