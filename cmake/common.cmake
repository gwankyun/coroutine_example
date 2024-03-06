function(get_WIN32_WINNT version)
  if(CMAKE_SYSTEM_VERSION)
    set(ver ${CMAKE_SYSTEM_VERSION}) # 10.0.19043
    string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver}) # 10.0
    string(REGEX MATCH "^([0-9]+)" verMajor ${ver}) # 10
    # Check for Windows 10, b/c we'll need to convert to hex 'A'.
    if("${verMajor}" MATCHES "10")
      set(verMajor "A")
      string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver}) # A.0
    endif()
    # Remove all remaining '.' characters.
    string(REPLACE "." "" ver ${ver}) # A0
    # Prepend each digit with a zero.
    string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver}) # 0A00
    set(${version} "0x${ver}" PARENT_SCOPE) # 0x0A00
  endif()
endfunction()

function(check_package name)
  message(CHECK_START "Looking for ${name}")
  if(${name}_FOUND)
    set(ver ${${name}_VERSION})
    message(CHECK_PASS "found version \"${ver}\"")
  else()
    message(CHECK_FAIL "not found")
  endif()
endfunction()
