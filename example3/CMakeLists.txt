# Note: This CMakeLists.txt is a minimal complete example how to use the
# *installed* application core library. It is not included from the main CMakeLists.txt
# which is used to build ApplicationCore itself.
CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)
#The regex matches everthing that is not a / until the end of the string (i.e. everyting after the last slash, if any)
string(REGEX MATCH "[^/]*$" PROJECT_BASE_NAME ${CMAKE_SOURCE_DIR})
message("REGEX MATCH is ${PROJECT_BASE_NAME}")

SET(PROJECT_NAME demo_${PROJECT_BASE_NAME})

# Use the project template to get the settings required for an application core project
set(${PROJECT_NAME}_MAJOR_VERSION 01)
set(${PROJECT_NAME}_MINOR_VERSION 00)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)
include(cmake/add_dependency.cmake)
include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)

# Add the dependencies. We need ApplicationCore and a control system adapter implementation.
add_dependency(ChimeraTK-ApplicationCore 01.06 REQUIRED)
#FIXME: Make the adapter configurable via command line parameter
add_dependency(ChimeraTK-ControlSystemAdapter-DoocsAdapter 00.10 REQUIRED)

AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR} demo_sources)

# The server proper. It depends on application core and the control system adapter implementation.
add_executable(${PROJECT_NAME} ${demo_sources})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${ChimeraTK-ApplicationCore_LINK_FLAGS} ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LINK_FLAGS}")
  target_link_libraries(${PROJECT_NAME} ${ChimeraTK-ApplicationCore_LIBRARIES} ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LIBRARIES} )

# We compile the same sources with the GENERATE_XML flag to get an xml generator.
# This one does not depent on a control system adapter implementation.
add_executable(${PROJECT_NAME}-xmlGenerator ${demo_sources})
set_target_properties( ${PROJECT_NAME}-xmlGenerator PROPERTIES COMPILE_FLAGS "-DGENERATE_XML")
set_target_properties(${PROJECT_NAME}-xmlGenerator PROPERTIES LINK_FLAGS "${ChimeraTK-ApplicationCore_LINK_FLAGS}")
target_link_libraries(${PROJECT_NAME}-xmlGenerator ${ChimeraTK-ApplicationCore_LIBRARIES})

# copy the (test) config files to the build directory for tests
FILE( COPY config/ DESTINATION ${PROJECT_BINARY_DIR})

# Tests:
# There are no dedicated tests for this demo. But we run the xml generator to
# check that the variable household can successfully be initialised.
# The test will fail if the xml generator crashes, just a smoke test.
ENABLE_TESTING()
add_test(${PROJECT_NAME}-xmlGenerator ${PROJECT_NAME}-xmlGenerator)

# Installation:
# FIXME: For doocs we need a special treatment when installing to /export/doocs/server (don't install to bin subdirectory, but a directory named like the server). This should go to the project template.
if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/export/doocs/server")
  install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-xmlGenerator RUNTIME DESTINATION ${PROJECT_NAME})
else()
  install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-xmlGenerator RUNTIME DESTINATION bin)
endif()
# Do not install the config. It is only a test config.
# The real config will come from a config generator and usually depends on the instance.
