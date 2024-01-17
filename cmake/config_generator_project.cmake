#
# cmake include to be used for config generator based projects.
#
# Configuration packages for servers can have a very simple CMakeLists.txt like this:
#
#   PROJECT(exampleserver-config NONE)
#   cmake_minimum_required(VERSION 3.14)
#
#   # Note: Always keep MAJOR_VERSION and MINOR_VERSION identical to the server version. Count only the patch separately.
#   set(${PROJECT_NAME}_MAJOR_VERSION 01)
#   set(${PROJECT_NAME}_MINOR_VERSION 00)
#   set(${PROJECT_NAME}_PATCH_VERSION 00)
#   include(cmake/set_version_numbers.cmake)
#
#   include(cmake/config_generator_project.cmake)
#
cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

find_package(ConfigGenerator 02.00 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${ConfigGenerator_DIR}/shared)

set(DESTDIR share/ConfigGenerator-${PROJECT_NAME}-${${PROJECT_NAME}_MAJOR_VERSION}-${${PROJECT_NAME}_MINOR_VERSION})

# find all server type directories in our source directory and copy them to the build directory
file(GLOB hostlists RELATIVE ${PROJECT_SOURCE_DIR} */hostlist)
foreach(hostlist ${hostlists})
  string(REPLACE "/hostlist" "" servertype "${hostlist}")
  # FOLLOW_SYMLINK_CHAIN is necessary to allow symlinks to git-submodules. This is used to create
  # "derived" configurations which reuse some files from their base configuration (example: configs for
  # different control systems)
  file(COPY "${PROJECT_SOURCE_DIR}/${servertype}" DESTINATION "${PROJECT_BINARY_DIR}" FOLLOW_SYMLINK_CHAIN)
  list(APPEND servertypes "${servertype}")
endforeach()

# install server types (scripts are installed by upstream config generator project)
foreach(servertype ${servertypes})
  install(DIRECTORY "${servertype}/settings" DESTINATION "${DESTDIR}/${servertype}")
  install(DIRECTORY "${servertype}/templates" DESTINATION "${DESTDIR}/${servertype}")
  file(GLOB thefiles LIST_DIRECTORIES no "${servertype}/*")
  install(FILES ${thefiles} DESTINATION "${DESTDIR}/${servertype}")
endforeach()
