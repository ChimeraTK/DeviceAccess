# CMakeLists.txt template for an ApplicationCore-based server
#
#
PROJECT(ChimeraTK-ApplicationCore-TemplateServer)
CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)

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

# Select the control system adapter
include(cmake/set_control_system_adapter.cmake)

# Define compiler and link flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ChimeraTK-ApplicationCore_CXX_FLAGS}")
set(CMAKE_LINK_FLAGS "-Wl,--no-as-needed ${Adapter_LINK_FLAGS} ${ChimeraTK-ApplicationCore_LINK_FLAGS}")



# configure version.h
configure_file(server/include/version.h.in ${PROJECT_BINARY_DIR}/version/version.h)
include_directories(${PROJECT_BINARY_DIR}/version)
# Module and Server include directories
include_directories(${CMAKE_SOURCE_DIR}/modules/include ${CMAKE_SOURCE_DIR}/server/include)

# Process server ApplicationModules
add_subdirectory(${CMAKE_SOURCE_DIR}/modules)

set(SERVER_SRC_DIR ${CMAKE_SOURCE_DIR}/server/src)

# The server executable. It depends on application core and the control system adapter implementation.
add_executable(${PROJECT_NAME} ${SERVER_SRC_DIR}/ApplicationInstance.cc ${SERVER_SRC_DIR}/Server.cc)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${CMAKE_LINK_FLAGS}")
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}lib ${ChimeraTK-ApplicationCore_LIBRARIES} ${Adapter_LIBRARIES} )

# We compile the same sources with the GENERATE_XML flag to get an xml generator.
# This one does not depent on a control system adapter implementation.
add_executable(${PROJECT_NAME}-xmlGenerator ${SERVER_SRC_DIR}/ApplicationInstance.cc ${SERVER_SRC_DIR}/Server.cc)
set_target_properties( ${PROJECT_NAME}-xmlGenerator PROPERTIES COMPILE_FLAGS "-DGENERATE_XML")
set_target_properties(${PROJECT_NAME}-xmlGenerator PROPERTIES LINK_FLAGS "${ChimeraTK-ApplicationCore_LINK_FLAGS}")
target_link_libraries(${PROJECT_NAME}-xmlGenerator ${PROJECT_NAME}lib ${ChimeraTK-ApplicationCore_LIBRARIES})

# Process tests:
ENABLE_TESTING()
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)

# Copy the (test) config files to the build directory
FILE( COPY ${CMAKE_SOURCE_DIR}/config/ DESTINATION ${PROJECT_BINARY_DIR})


# Installation:
# FIXME: For doocs we need a special treatment when installing to /export/doocs/server (don't install to bin subdirectory, but a directory named like the server). This should go to the project template.
if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/export/doocs/server")
  install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-xmlGenerator RUNTIME DESTINATION ${PROJECT_NAME})
else()
  install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-xmlGenerator RUNTIME DESTINATION bin)
endif()
# Do not install the config. It is only a test config.
# The real config will come from a config generator and usually depends on the instance.
