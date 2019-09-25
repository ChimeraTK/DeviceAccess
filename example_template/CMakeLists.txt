# CMakeLists.txt template for an ApplicationCore-based server
# *installed* application core library. It is not included from the main CMakeLists.txt
# which is used to build ApplicationCore itself.
PROJECT(ChimeraTK-ApplicationCore-TemplateServer)
CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)

#SET(PROJECT_NAME demo_${PROJECT_BASE_NAME})

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
add_dependency(ChimeraTK-ApplicationCore 01.08 REQUIRED)

#FIXME: Make the adapter configurable via command line parameter
#add_dependency(ChimeraTK-ControlSystemAdapter-DoocsAdapter 00.10 REQUIRED)
IF(ADAPTER STREQUAL "OPCUA")
  message("Building against the OPC UA ControlSystemAdater")
  add_dependency(ChimeraTK-ControlSystemAdapter-OPCUAAdapter REQUIRED)
  set(Adapter_LINK_FLAGS ${ChimeraTK-ControlSystemAdapter-OPCUAAdapter_LINK_FLAGS})
  set(Adapter_LIBRARIES ${ChimeraTK-ControlSystemAdapter-OPCUAAdapter_LIBRARIES})
ELSEIF(ADAPTER STREQUAL "DOOCS")
  message("Building against the DOOCS ControlSystemAdater")
  add_dependency(ChimeraTK-ControlSystemAdapter-DoocsAdapter REQUIRED)
  set(Adapter_LINK_FLAGS ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LINK_FLAGS})
  set(Adapter_LIBRARIES ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LIBRARIES})
ELSEIF(ADAPTER STREQUAL "EPICSIOC")
  message("Building against the EPICS IOC ControlSystemAdater")
  add_dependency(ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter REQUIRED)
  set(Adapter_LINK_FLAGS ${ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter_LINK_FLAGS})
  set(Adapter_LIBRARIES ${ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter_LIBRARIES})
ELSE()
  message(FATAL_ERROR "Please select your ControlSystemAdapter to use by passing to the cmake command line:\n"
                      "   -DADAPTER=DOOCS to build a DOOCS server\n"
                      "   -DADAPTER=OPCUA to build an OPC UA server\n"
                      "   -DADAPTER=EPICSIOC to build an EPICS IOC")
ENDIF()


# configure version.h
configure_file(include/version.h.in ${PROJECT_BINARY_DIR}/version/version.h)
include_directories(${PROJECT_BINARY_DIR}/version)

include_directories(${CMAKE_SOURCE_DIR}/include)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/src server_sources)

# The server proper. It depends on application core and the control system adapter implementation.
add_executable(${PROJECT_NAME} ${server_sources})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${ChimeraTK-ApplicationCore_LINK_FLAGS} ${Adapter_LINK_FLAGS}")
  target_link_libraries(${PROJECT_NAME} ${ChimeraTK-ApplicationCore_LIBRARIES} ${Adapter_LIBRARIES} )

# We compile the same sources with the GENERATE_XML flag to get an xml generator.
# This one does not depent on a control system adapter implementation.
add_executable(${PROJECT_NAME}-xmlGenerator ${server_sources})
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
