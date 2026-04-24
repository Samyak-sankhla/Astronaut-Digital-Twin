#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Biogears::libbiogears_common" for configuration "Release"
set_property(TARGET Biogears::libbiogears_common APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Biogears::libbiogears_common PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib//libbiogears_common_st.a"
  )

list(APPEND _cmake_import_check_targets Biogears::libbiogears_common )
list(APPEND _cmake_import_check_files_for_Biogears::libbiogears_common "${_IMPORT_PREFIX}/lib//libbiogears_common_st.a" )

# Import target "Biogears::libbiogears_io" for configuration "Release"
set_property(TARGET Biogears::libbiogears_io APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Biogears::libbiogears_io PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib//libbiogears_io.so.8.2"
  IMPORTED_SONAME_RELEASE "libbiogears_io.so.8.2"
  )

list(APPEND _cmake_import_check_targets Biogears::libbiogears_io )
list(APPEND _cmake_import_check_files_for_Biogears::libbiogears_io "${_IMPORT_PREFIX}/lib//libbiogears_io.so.8.2" )

# Import target "Biogears::libbiogears_cdm" for configuration "Release"
set_property(TARGET Biogears::libbiogears_cdm APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Biogears::libbiogears_cdm PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib//libbiogears_cdm.so.8.2"
  IMPORTED_SONAME_RELEASE "libbiogears_cdm.so.8.2"
  )

list(APPEND _cmake_import_check_targets Biogears::libbiogears_cdm )
list(APPEND _cmake_import_check_files_for_Biogears::libbiogears_cdm "${_IMPORT_PREFIX}/lib//libbiogears_cdm.so.8.2" )

# Import target "Biogears::libbiogears" for configuration "Release"
set_property(TARGET Biogears::libbiogears APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Biogears::libbiogears PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib//libbiogears.so.8.2"
  IMPORTED_SONAME_RELEASE "libbiogears.so.8.2"
  )

list(APPEND _cmake_import_check_targets Biogears::libbiogears )
list(APPEND _cmake_import_check_files_for_Biogears::libbiogears "${_IMPORT_PREFIX}/lib//libbiogears.so.8.2" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
