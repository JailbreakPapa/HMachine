#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "lz4::lz4" for configuration "Release"
set_property(TARGET lz4::lz4 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(lz4::lz4 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/lz4.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS lz4::lz4 )
list(APPEND _IMPORT_CHECK_FILES_FOR_lz4::lz4 "${_IMPORT_PREFIX}/lib/lz4.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
