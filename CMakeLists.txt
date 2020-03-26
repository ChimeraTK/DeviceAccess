PROJECT(ChimeraTK-ApplicationCore)
cmake_minimum_required(VERSION 2.8)

include(cmake/add_dependency.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 01)
set(${PROJECT_NAME}_MINOR_VERSION 12)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

option(BUILD_TESTS "Build tests." ON)
option(BUILD_MICRODAQ "Build MicroDAQ module, which depends on HDF5 (libhdf5-dev)." ON)

# Find the ControlSystemAdapter
add_dependency(ChimeraTK-ControlSystemAdapter 01.06 REQUIRED)

# Find the DeviceAccess
# Although all the linker flags are added again, the include path might be different
# if the libraries are installed in different directories for development purposes.
add_dependency(ChimeraTK-DeviceAccess 01.11 REQUIRED)

# Find the XML parser library libxml++
FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED glib-2.0)
include_directories(SYSTEM ${glib_INCLUDE_DIRS} ${LibXML++_INCLUDE_DIRS})

# optional dependency: BOOST unit test framework needed for tests
FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
IF(BUILD_TESTS)
  IF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
    set(TESTING_IS_ENABLED "true")
    ENABLE_TESTING()
  ELSE()
    message(" The following packages were not found, so testing will be disabled:")
    message("  * BOOST unit_test_framework")
  ENDIF()
ENDIF(BUILD_TESTS)

# Find BOOST filesystem
# Note: we need to search this after looking for the unit_test_framework, since we must not link against the
#       unit_test_framework library (use header-only)
FIND_PACKAGE(Boost COMPONENTS filesystem date_time chrono system thread REQUIRED)

# requierd dependency: HDF5 library needed for the MicroDAQ system (TODO make optional!)
option(BUILD_MICRODAQ "Build the MicroDAQ module, which depends on HDF5 (libhdf5-dev)." ON)
if(BUILD_MICRODAQ)
  FIND_PACKAGE(HDF5 COMPONENTS CXX)
  IF(HDF5_FOUND)
    include_directories(SYSTEM ${HDF5_INCLUDE_DIRS})
    link_directories(${HDF5_LIBRARY_DIRS})
    # add flag to be used in user code to test weather MicroDAQ is included
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_MICRO_DAQ")
  ELSE()
    set(HDF5_LIBRARIES "")
    message(" HDF5 was not found, so the MicroDAQ system will not be built.")
  ENDIF()
endif(BUILD_MICRODAQ)

include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)

# linker flags
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-dynamic ${ChimeraTK-ControlSystemAdapter_LINK_FLAGS}")

include_directories(${CMAKE_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/ ${CMAKE_SOURCE_DIR}/Modules/include/)
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

# add generic modules
include_directories(${CMAKE_SOURCE_DIR}/Modules/include)
aux_source_directory(${CMAKE_SOURCE_DIR}/Modules/src library_sources)

if((NOT BUILD_MICRODAQ) OR (NOT HDF5_FOUND))
  LIST(REMOVE_ITEM library_sources ${CMAKE_SOURCE_DIR}/Modules/include/MicroDAQ.h)
  LIST(REMOVE_ITEM library_sources ${CMAKE_SOURCE_DIR}/Modules/src/MicroDAQ.cc)
endif((NOT BUILD_MICRODAQ) OR (NOT HDF5_FOUND))

MACRO( COPY_MAPPING_FILES )
  foreach( FILE_TO_COPY test.xlmap test.dmap )
    configure_file( tests/${FILE_TO_COPY} ${PROJECT_BINARY_DIR}/${FILE_TO_COPY} copyonly)
  endforeach( FILE_TO_COPY )
ENDMACRO( COPY_MAPPING_FILES )

# Create the executables for automated unit testing.
if(TESTING_IS_ENABLED)
  include_directories(${CMAKE_SOURCE_DIR}/tests/include)
  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/executables_src testExecutables)
  foreach( testExecutableSrcFile ${testExecutables})
    #NAME_WE means the base name without path and (longest) extension
    get_filename_component(executableName ${testExecutableSrcFile} NAME_WE)
    add_executable(${executableName} ${testExecutableSrcFile} )
    # do not link against the boost unit test library, the tests are not written for it!
    target_link_libraries(${executableName} ${PROJECT_NAME} ${ChimeraTK-ControlSystemAdapter_LIBRARIES} ${HDF5_LIBRARIES})
    set_target_properties(${executableName} PROPERTIES LINK_FLAGS "-Wl,-rpath,${PROJECT_BINARY_DIR} ${Boost_LINK_FLAGS} ${ChimeraTK-ControlSystemAdapter_LINK_FLAGS}")
    add_test(${executableName} ${executableName})
  endforeach( testExecutableSrcFile )

  # enable code coverate report
  include(cmake/enable_code_coverage_report.cmake)

  # copy config files
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test.xlmap DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test2.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test3.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test4.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test5.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/testDataValidity1.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/testDataValidity2.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/test.dmap DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/configReaderDevice.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/tests/validConfig.xml DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/xmlschema/application.xsd DESTINATION ${PROJECT_BINARY_DIR})

endif(TESTING_IS_ENABLED)

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION}
                                                 SOVERSION ${${PROJECT_NAME}_SOVERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${ChimeraTK-ControlSystemAdapter_LINK_FLAGS}")
target_link_libraries(${PROJECT_NAME} ${ChimeraTK-ControlSystemAdapter_LIBRARIES}
                                      ${Boost_LIBRARIES}
                                      pthread
                                      ${LibXML++_LIBRARIES}
                                      ${glib_LIBRARIES}
                                      ${HDF5_LIBRARIES})

# do not remove runtime path of the library when installing
set_property(TARGET ${PROJECT_NAME} PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

# add a target to generate API documentation with Doxygen
include(cmake/enable_doxygen_documentation.cmake)

# Install the library and the executables
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib )

# all include files go into include/PROJECT_NAME
# The exclusion of ${PROJECT_NAME} prevents the recursive installation of the files just being installed.
# The original headers are in include/*.h, the installed ones in include/PROJECT_NAME/*.h.
if((NOT BUILD_MICRODAQ) OR (NOT HDF5_FOUND))
  install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/ChimeraTK/ApplicationCore
                                                    FILES_MATCHING PATTERN "*.h"
                                                                   PATTERN ".svn" EXCLUDE
                                                                   PATTERN "${PROJECT_NAME}" EXCLUDE
                                                                   PATTERN "*MicroDAQ.h" EXCLUDE
                                                    )
else()
  install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/ChimeraTK/ApplicationCore
                                                    FILES_MATCHING PATTERN "*.h"
                                                                   PATTERN ".svn" EXCLUDE
                                                                   PATTERN "${PROJECT_NAME}" EXCLUDE)
endif((NOT BUILD_MICRODAQ) OR (NOT HDF5_FOUND))

set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
set(${PROJECT_NAME}_LIBRARIES "${ChimeraTK-ControlSystemAdapter_LIBRARIES} ${HDF5_LIBRARIES}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib")
set(${PROJECT_NAME}_CXX_FLAGS "${ChimeraTK-ControlSystemAdapter_CXX_FLAGS}")
set(${PROJECT_NAME}_LINK_FLAGS "${ChimeraTK-ControlSystemAdapter_LINK_FLAGS}")

list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "Boost COMPONENTS system thread chrono filesystem date_time atomic")
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)

