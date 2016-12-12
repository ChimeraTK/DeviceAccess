PROJECT(ChimeraTK-ApplicationCore)
cmake_minimum_required(VERSION 2.8)

include(cmake/add_dependency.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 01)
set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})

# This library depends on deviceaccess, which is likely to change often.
# We add a build version to the version number to be able to install
# the same software version of the library linked against different versions
# of deviceacces (or other dependencies)
set(${PROJECT_NAME}_BUILDVERSION $ENV{${PROJECT_NAME}_BUILDVERSION})
if( ${PROJECT_NAME}_BUILDVERSION )
  set(${PROJECT_NAME}_SOVERSION "${${PROJECT_NAME}_SOVERSION}${${PROJECT_NAME}_BUILDVERSION}")
endif( ${PROJECT_NAME}_BUILDVERSION )
# We need the build name to be part of the libraries full version name
set(${PROJECT_NAME}_FULL_LIBRARY_VERSION ${${PROJECT_NAME}_SOVERSION}.${${PROJECT_NAME}_PATCH_VERSION})

# Find the DeviceAccess library
# put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
add_dependency(mtca4u-deviceaccess 00.18 REQUIRED)

# Find the ControlSystemAdapter
# put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
add_dependency(ChimeraTK-ControlSystemAdapter 00.03 REQUIRED)

# Find the XML parser library libxml++
FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED glib-2.0)
include_directories(SYSTEM "${LibXML++_INCLUDEDIR}/${LIBXML++_VERSION}" ${glib_INCLUDE_DIRS} ${LibXML++_INCLUDE_DIRS})

# optional dependency: BOOST unit test framework needed for tests
FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
IF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  set(TESTING_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE()
  message(" The following packages were not found, so testing will be disabled:")
  message("  * BOOST unit_test_framework")
ENDIF()

# optional dependency: the ControlSystemAdapter-DoocsAdapter is needed for the example
add_dependency(ControlSystemAdapter-DoocsAdapter 00.01)
IF(ControlSystemAdapter-DoocsAdapter_FOUND)
  set(DEMO_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE()
  message(" The following packages were not found, so the example will be disabled:")
  IF(NOT ControlSystemAdapter-DoocsAdapter_FOUND)
    message("  * ControlSystemAdapter-DoocsAdapter")
  ENDIF()
ENDIF()

# Moderate version of the compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wextra -Wuninitialized -O3")

# use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")

# linker flags
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-dynamic ${mtca4u-deviceaccess_LINK_FLAGS}")

include_directories(${CMAKE_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/)
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

MACRO( COPY_MAPPING_FILES )
  foreach( FILE_TO_COPY test.xlmap test.dmap )
    configure_file( tests/${FILE_TO_COPY} ${PROJECT_BINARY_DIR}/${FILE_TO_COPY} copyonly)
  endforeach( FILE_TO_COPY )
ENDMACRO( COPY_MAPPING_FILES )

# Create the executables for automated unit testing.
if(TESTING_IS_ENABLED)

  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/executables_src testExecutables)
  foreach( testExecutableSrcFile ${testExecutables})
    #NAME_WE means the base name without path and (longest) extension
    get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
    add_executable(${excutableName} ${testExecutableSrcFile})
    target_link_libraries(${excutableName} ${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES} ${ChimeraTK-ControlSystemAdapter_LIBRARIES} ${Boost_LIBRARIES} ${LibXML++_LIBRARIES} ${glib_LIBRARIES})
    add_test(${excutableName} ${excutableName})
  endforeach( testExecutableSrcFile )

  #The make coverage command is only available in debug mode
  IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    configure_file(cmake/Makefile.coverage.in
      ${PROJECT_BINARY_DIR}/Makefile.coverage @ONLY)
    add_custom_target(coverage
      make -f Makefile.coverage
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating test coverage documentation" VERBATIM
      )
  ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

  # copy config files
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/dummy.dmap DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/dummy.xlmap DESTINATION ${PROJECT_BINARY_DIR})

endif(TESTING_IS_ENABLED)

# build example
if(DEMO_IS_ENABLED)

  add_executable(demoApp example/demoApp.cc)
  set_target_properties(demoApp PROPERTIES COMPILE_FLAGS "${mtca4u-deviceaccess_CXX_FLAGS} ${ControlSystemAdapter_CXX_FLAGS}")
  set_target_properties(demoApp PROPERTIES LINK_FLAGS "${mtca4u-deviceaccess_LINK_FLAGS} ${ControlSystemAdapter_LINK_FLAGS} ${ControlSystemAdapter-DoocsAdapter_LINK_FLAGS}")
  target_link_libraries(demoApp ${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES} ${ChimeraTK-ControlSystemAdapter_LIBRARIES} ${ControlSystemAdapter-DoocsAdapter_LIBRARIES} ${LibXML++_LIBRARIES} ${glib_LIBRARIES})

  add_executable(demoApp-xmlGenerator example/demoApp.cc)
  set_target_properties(demoApp-xmlGenerator PROPERTIES COMPILE_FLAGS "${mtca4u-deviceaccess_CXX_FLAGS} -DGENERATE_XML")
  target_link_libraries(demoApp-xmlGenerator ${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES} ${ChimeraTK-ControlSystemAdapter_LIBRARIES} ${LibXML++_LIBRARIES} ${glib_LIBRARIES})
  
  add_custom_target(demoApp-xml ALL DEPENDS demoApp-xmlGenerator COMMAND ${PROJECT_BINARY_DIR}/demoApp-xmlGenerator WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

  # copy config files
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/demoApp.conf DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/dummy.dmap DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/dummy.xlmap DESTINATION ${PROJECT_BINARY_DIR})

endif(DEMO_IS_ENABLED)

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES} ${ChimeraTK-ControlSystemAdapter_LIBRARIES} ${Boost_LIBRARIES} ${LibXML++_LIBRARIES} ${glib_LIBRARIES} pthread)

# do not remove runtime path of the library when installing
set_property(TARGET ${PROJECT_NAME} PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/ DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION})
else(DOXYGEN_FOUND)
  message("Doxygen not found, documentation will not be built.")
endif(DOXYGEN_FOUND)

# Install the library and the executables
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib )

# all include files go into include/PROJECT_NAME
# The exclusion of ${PROJECT_NAME} prevents the recursive installation of the files just being installed.
# The original headers are in include/*.h, the installed ones in include/PROJECT_NAME/*.h.
install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/ChimeraTK/ApplicationCore
        FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE PATTERN "${PROJECT_NAME}" EXCLUDE )

set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
set(${PROJECT_NAME}_LIBRARIES "${PROJECT_NAME}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib")
set(${PROJECT_NAME}_CXX_FLAGS "${mtca4u-deviceaccess_CXX_FLAGS} ${ControlSystemAdapter_CXX_FLAGS}")
set(${PROJECT_NAME}_LINK_FLAGS "${mtca4u-deviceaccess_LINK_FLAGS} ${ControlSystemAdapter_LINK_FLAGS} ${ControlSystemAdapter-DoocsAdapter_LINK_FLAGS}")
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)

