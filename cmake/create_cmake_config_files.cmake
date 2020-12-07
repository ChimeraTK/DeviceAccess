#######################################################################################################################
# create_cmake_config_files.cmake
#
# Create the Find${PROJECT_NAME}.cmake cmake macro and the ${PROJECT_NAME}-config shell script and installs them.
#
# Expects the following input variables:
#   ${PROJECT_NAME}_SOVERSION - version of the .so library file (or just MAJOR.MINOR without the patch level)
#   ${PROJECT_NAME}_INCLUDE_DIRS - list include directories needed when compiling against this project
#   ${PROJECT_NAME}_LIBRARY_DIRS - list of library directories needed when linking against this project
#   ${PROJECT_NAME}_LIBRARIES - list of additional libraries needed when linking against this project. The library
#                               provided by the project will be added automatically
#   ${PROJECT_NAME}_CXX_FLAGS - list of additional C++ compiler flags needed when compiling against this project
#   ${PROJECT_NAME}_LINKER_FLAGS - list of additional linker flags needed when linking against this project
#   ${PROJECT_NAME}_MEXFLAGS - (optional) mex compiler flags
#
#######################################################################################################################

#######################################################################################################################
#
# IMPORTANT NOTE:
#
# DO NOT MODIFY THIS FILE inside a project. Instead update the project-template repository and pull the change from
# there. Make sure to keep the file generic, since it will be used by other projects, too.
#
# If you have modified this file inside a project despite this warning, make sure to cherry-pick all your changes
# into the project-template repository immediately.
#
#######################################################################################################################

# create variables for standard makefiles
set(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "${${PROJECT_NAME}_CXX_FLAGS}")

string(REPLACE " " ";" LIST "${${PROJECT_NAME}_INCLUDE_DIRS}")
foreach(INCLUDE_DIR ${LIST})
  set(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "${${PROJECT_NAME}_CXX_FLAGS_MAKEFILE} -I${INCLUDE_DIR}")
endforeach()

set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS} ${${PROJECT_NAME}_LINK_FLAGS}")

string(REPLACE " " ";" LIST "${${PROJECT_NAME}_LIBRARY_DIRS}")
foreach(LIBRARY_DIR ${LIST})
  set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} -L${LIBRARY_DIR}")
endforeach()

string(REPLACE " " ";" LIST "${PROJECT_NAME} ${${PROJECT_NAME}_LIBRARIES}")
foreach(LIBRARY ${LIST})
  if(LIBRARY MATCHES "/")         # library name contains slashes: link against the a file path name
    set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} ${LIBRARY}")
  elseif(LIBRARY MATCHES "^-l")   # library name does not contain slashes but already the -l option: directly quote it
    set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} ${LIBRARY}")
  elseif(LIBRARY MATCHES "::")  # library name is an exported target - we need to resolve it for Makefiles
    get_property(lib_loc TARGET ${LIBRARY} PROPERTY LOCATION)
    string(APPEND ${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE " ${lib_loc}")
  else()                          # link against library with -l option
    set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} -l${LIBRARY}")
  endif()
endforeach()

set(${PROJECT_NAME}_PUBLIC_DEPENDENCIES_L "")
foreach(DEPENDENCY ${${PROJECT_NAME}_PUBLIC_DEPENDENCIES})
    string(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES_L "find_package(${DEPENDENCY} REQUIRED)\n")
endforeach()

if(TARGET ${PROJECT_NAME})
  set(${PROJECT_NAME}_HAS_LIBRARY 1)
else()
  set(${PROJECT_NAME}_HAS_LIBRARY 0)
endif()

# we have nested @-statements, so we have to parse twice:

# create the cmake find_package configuration file
configure_file(cmake/PROJECT_NAMEConfig.cmake.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)
configure_file(cmake/PROJECT_NAMEConfigVersion.cmake.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)

# create the shell script for standard make files
configure_file(cmake/PROJECT_NAME-config.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config" @ONLY)

# install cmake find_package configuration file
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  DESTINATION lib/cmake/${PROJECT_NAME} COMPONENT dev)
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION lib/cmake/${PROJECT_NAME} COMPONENT dev)

# install same cmake configuration file another time into the Modules cmake subdirectory for compatibility reasons
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  DESTINATION share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules RENAME Find${PROJECT_NAME}.cmake COMPONENT dev)

# install script for Makefiles
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config DESTINATION bin COMPONENT dev)

