#
# cmake include to be used for config generator based projects.
#
# Configuration packages for servers can have a very simple CMakeLists.txt like this:
#
#   PROJECT(exampleserver-config NONE)
#   cmake_minimum_required(VERSION 3.5)
#
#   # Note: Always keep MAJOR_VERSION and MINOR_VERSION identical to the server version. Count only the patch separately.
#   set(${PROJECT_NAME}_MAJOR_VERSION 01)
#   set(${PROJECT_NAME}_MINOR_VERSION 00)
#   set(${PROJECT_NAME}_PATCH_VERSION 00)
#   include(cmake/set_version_numbers.cmake)
#
#   include(cmake/config_generator_project.cmake)
#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

find_package(ConfigGenerator 02.00 REQUIRED)

set(DESTDIR share/ConfigGenerator-${PROJECT_NAME}-${${PROJECT_NAME}_MAJOR_VERSION}-${${PROJECT_NAME}_MINOR_VERSION})

# copy all script files from config generator to our build directory
file(GLOB scripts RELATIVE ${ConfigGenerator_DIR} ${ConfigGenerator_DIR}/*.sh ${ConfigGenerator_DIR}/*.py
                           ${ConfigGenerator_DIR}/*.inc ${ConfigGenerator_DIR}/ConfigGenerator/*.py
                           ${ConfigGenerator_DIR}/TestFacility/*.py)
foreach(script ${scripts})
  configure_file("${ConfigGenerator_DIR}/${script}" "${PROJECT_BINARY_DIR}/${script}" COPYONLY)
endforeach()

# find all server type directories in our source directory
file(GLOB hostlists RELATIVE ${PROJECT_SOURCE_DIR} */hostlist)
foreach(hostlist ${hostlists})
  string(REPLACE "/hostlist" "" servertype "${hostlist}")
  list(APPEND servertypes "${servertype}")
endforeach()

# copy all server type directories to the build directory
foreach(servertype ${servertypes})
  file(GLOB_RECURSE files RELATIVE ${PROJECT_SOURCE_DIR} ${servertype}/*)
  foreach(f ${files})
    configure_file("${f}" "${PROJECT_BINARY_DIR}/${f}" COPYONLY)
  endforeach()
endforeach()

# install server types (scripts are installed by upstream config generator project)
foreach(servertype ${servertypes})
  install(DIRECTORY "${servertype}/settings" DESTINATION "${DESTDIR}/${servertype}")
  install(DIRECTORY "${servertype}/templates" DESTINATION "${DESTDIR}/${servertype}")
  file(GLOB thefiles LIST_DIRECTORIES no "${servertype}/*")
  install(FILES ${thefiles} DESTINATION "${DESTDIR}/${servertype}")
endforeach()

