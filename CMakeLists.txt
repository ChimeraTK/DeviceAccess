PROJECT(ChimeraTK-ControlSystemDeviceConnector)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 00)
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

# put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
set(mtca4u-deviceaccess_MIN_VERSION 00.12)
FIND_PACKAGE(mtca4u-deviceaccess ${mtca4u-deviceaccess_MIN_VERSION} REQUIRED)
include_directories(SYSTEM ${mtca4u-deviceaccess_INCLUDE_DIRS})

# put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
set(ControlSystemAdapter_MIN_VERSION 00.00)
FIND_PACKAGE(ControlSystemAdapter ${ControlSystemAdapter_MIN_VERSION} REQUIRED)
include_directories(SYSTEM ${ControlSystemAdapter_INCLUDE_DIRS})

# optional dependency: if finding the DOOCS server library, the example and tests will be built
# put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
set(DOOCS_MIN_VERSION 18.10.5)
FIND_PACKAGE(DOOCS ${DOOCS_MIN_VERSION} COMPONENTS server)
include_directories(SYSTEM ${DOOCS_INCLUDE_DIRS})
link_directories(${DOOCS_LIBRARY_DIRS})

# optional dependency: the DoocsServerTestHelper is also needed for the tests
FIND_PACKAGE(mtca4u-doocsServerTestHelper)
include_directories(SYSTEM ${mtca4u-doocsServerTestHelper_INCLUDE_DIRS})
link_directories(${mtca4u-doocsServerTestHelper_LIBRARY_DIRS})

# optional dependency: the ControlSystemAdapter-DoocsAdapter needed for tests and example
FIND_PACKAGE(ControlSystemAdapter-DoocsAdapter)
include_directories(SYSTEM ${ControlSystemAdapter-DoocsAdapter_INCLUDE_DIRS})
link_directories(${ControlSystemAdapter-DoocsAdapter_LIBRARY_DIRS})

# optional dependency: BOOST unit test framework needed for tests
FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
IF(Boost_UNIT_TEST_FRAMEWORK_FOUND AND DOOCS_FOUND AND mtca4u-doocsServerTestHelper_FOUND)
  set(TESTING_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE()
  message(" One of the following packages was not found, so testing will be disabled:")
  message("  * BOOST unit_test_framework")
  message("  * DOOCS server lib")
  message("  * mtca4u-doocsServerTestHelper")
ENDIF()

# Moderate version of the compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wextra -ansi -Wuninitialized ${mtca4u-deviceaccess_CXX_FLAGS} -O3")

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
    target_link_libraries(${excutableName} ${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES} ${Boost_LIBRARIES})
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

endif(TESTING_IS_ENABLED)

# build example
if(TESTING_IS_ENABLED)

  add_executable(demoApp example/demoApp.cc)
  set_target_properties(demoApp PROPERTIES COMPILE_FLAGS "${mtca4u-deviceaccess_CXX_FLAGS} ${DOOCS_CXX_FLAGS}")
  set_target_properties(demoApp PROPERTIES LINK_FLAGS ${DOOCS_LINK_FLAGS})
  target_link_libraries(demoApp ${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES} ${ControlSystemAdapter-DoocsAdapter_LIBRARIES} ${DOOCS_LIBRARIES})

  # copy config file
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/demoApp.conf DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/dummy.dmap DESTINATION ${PROJECT_BINARY_DIR})
  FILE( COPY ${CMAKE_SOURCE_DIR}/example/dummy.xlmap DESTINATION ${PROJECT_BINARY_DIR})

endif(TESTING_IS_ENABLED)

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES})

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
install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/mtca4u/InstaCoSADev
        FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE PATTERN "${PROJECT_NAME}" EXCLUDE )

set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
set(${PROJECT_NAME}_LIBRARIES "${PROJECT_NAME}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib")
set(${PROJECT_NAME}_CXX_FLAGS "${mtca4u-deviceaccess_CXX_FLAGS}")
set(${PROJECT_NAME}_LINK_FLAGS "${mtca4u-deviceaccess_LINK_FLAGS}")
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)

# The instructions to create a debian package are loaded from a separate file
include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)
