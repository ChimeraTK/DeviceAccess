#######################################################################################################################
# create_cmake_config_files.cmake
#
# Create the Find${PROJECT_NAME}.cmake cmake macro and the ${PROJECT_NAME}-config shell script and installs them.
#
# Expects the following input variables:
#   ${PROJECT_NAME}_SOVERSION - version of the .so library file (or just MAJOR.MINOR without the patch level)
#   ${PROJECT_NAME}_MEXFLAGS - (optional) mex compiler flags
#
# and only required, if project does not yet provide cmake-export:
#   ${PROJECT_NAME}_INCLUDE_DIRS - list include directories needed when compiling against this project
#   ${PROJECT_NAME}_LIBRARY_DIRS - list of library directories needed when linking against this project
#   ${PROJECT_NAME}_LIBRARIES - list of additional libraries needed when linking against this project. The library
#                               provided by the project will be added automatically
#   ${PROJECT_NAME}_CXX_FLAGS - list of additional C++ compiler flags needed when compiling against this project
#   ${PROJECT_NAME}_LINKER_FLAGS - list of additional linker flags needed when linking against this project
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

macro(handleGeneratorExprs var)
    # Unfortunately, I do not see a solution for correct generator expression handling by cmake. So instead, do some simple replacements.
    # $<INSTALL_INTERFACE:path> defines path relative to install location.
    string(REGEX REPLACE "\\$<INSTALL_INTERFACE:([^ ;]*)>" "${CMAKE_INSTALL_PREFIX}/\\1" ${var} "${${var}}")
    # remove any other generator expression
    string(REGEX REPLACE "\\$<.*>" "" ${var} "${${var}}")
endmacro()

# append element-wise to (space-separated!) list, but only if not yet existing
# arg is allowed to be space-separated or ;-separated list
macro(appendToList list arg)
    string(REPLACE " " ";" appendToList_args "${arg}")
    foreach(item ${appendToList_args})
        handleGeneratorExprs(item)
        string(FIND " ${${list}} " " ${item} " appendToList_pos)
        if (${appendToList_pos} EQUAL -1)
            string(APPEND ${list} " ${item}")
            # strip leading spaces since they might cause problems
            string(REGEX REPLACE "^[ \t]+" "" ${list} "${${list}}")
        endif()
    endforeach()
endmacro()
# prepend element-wise to (space-separated!) list, but only if not yet existing
# arg is allowed to be space-separated or ;-separated list
macro(prependToList list arg)
    string(REPLACE " " ";" prependToList_args "${arg}")
    foreach(item ${prependToList_args})
        handleGeneratorExprs(item)
        string(FIND " ${${list}} " " ${item} " prependToList_pos)
        if (${prependToList_pos} EQUAL -1)
            string(PREPEND ${list} "${item} ")
            # strip trailing spaces since they might cause problems
            string(REGEX REPLACE "[ \t]+$" "" ${list} "${${list}}")
        endif()
    endforeach()
endmacro()

# for lib, which might be lib File or linker flag or imported target, 
# puts recursively resolved library list into ${linkLibs}, which will contain a library file list
# and recursively resolve link flags into ${linkFlags}
# Note, since some projects ignore libDirs, we put them also into linkFlags.
function(resolveImportedLib lib linkLibs linkFlags libDirs incDirs cxxFlags)
    set(linkLibs1 "")
    set(linkFlags1 "")
    set(libDirs1 "")
    set(incDirs1 "")
    set(cxxFlags1 "")
    if(lib MATCHES "/")         # library name contains slashes: link against the a file path name
        appendToList(linkLibs1 "${lib}")
    elseif(lib MATCHES "^[ \t]*-l")
        # library name does not contain slashes but already the -l option: directly quote it
        # although technically a linker flag, we put it to lib list because order matters sometimes
        appendToList(linkLibs1 "${lib}")
    elseif(lib MATCHES "::")    # library name is an imported target - we need to resolve it for Makefiles
        if (NOT TARGET ${lib})
            message(FATAL_ERROR "dependency ${lib} not available as target, maybe find_package was forgotten?")
        endif()
        get_target_property(_libraryType ${lib} TYPE)
        # boost exports appear as UNKNOWN_LIBRARY but also have target location
        if ((${_libraryType} MATCHES SHARED_LIBRARY) OR (${_libraryType} MATCHES STATIC_LIBRARY) OR (${_libraryType} MATCHES UNKNOWN_LIBRARY))
            if(";${lib};" MATCHES ";.*::${PROJECT_NAME};")
                # We cannot find target library location of this project via target properties at this point.
                # Therefore, we simply assume that by convention, all our libs are installed into ${CMAKE_INSTALL_PREFIX}/lib.
                # Exceptions are allowed if -L<libdir> is already in linker flags
                appendToList(linkFlags1 "-L${CMAKE_INSTALL_PREFIX}/lib")
                appendToList(libDirs1 "${CMAKE_INSTALL_PREFIX}/lib")
                appendToList(linkLibs1 "-l${PROJECT_NAME}")
            else()
                get_property(lib_loc TARGET ${lib} PROPERTY LOCATION)
                #message("imported target ${lib} is actual library. location=${lib_loc}")
                appendToList(linkLibs1 "${lib_loc}")
            endif()
        endif()
        get_target_property(_linkLibs ${lib} INTERFACE_LINK_LIBRARIES)
        if (NOT "${_linkLibs}" MATCHES "-NOTFOUND")
            message(VERBOSE "imported target ${lib} is interface, recursively go over its interface requirements ${_linkLibs}")
            foreach(_lib ${_linkLibs})
                if (${lib} STREQUAL ${_lib})
                    message(FATAL_ERROR "self-reference in dependencies of ${_lib}! Aborting recursion.")
                endif()
                resolveImportedLib(${_lib} linkLibs2 linkFlags2 libDirs2 incDirs2 cxxFlags2)
                appendToList(linkLibs1 "${linkLibs2}")
                appendToList(linkFlags1 "${linkFlags2}")
                appendToList(libDirs1 "${libDirs2}")
                appendToList(incDirs1 "${incDirs2}")
                appendToList(cxxFlags1 "${cxxFlags2}")
            endforeach()
        endif()

        get_target_property(_incDirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
        if (_incDirs)
            appendToList(incDirs1 "${_incDirs}")
        endif()
        get_target_property(_cxxFlags ${lib} INTERFACE_COMPILE_OPTIONS)
        if (_cxxFlags)
            appendToList(cxxFlags1 "${_cxxFlags}")
        endif()
        get_target_property(_cxxFlags ${lib} INTERFACE_COMPILE_DEFINITIONS)
        if (_cxxFlags)
            foreach(flag ${_cxxFlags})
                appendToList(cxxFlags1 "-D${flag}")
            endforeach()
        endif()
        get_target_property(_linkFlags ${lib} INTERFACE_LINK_OPTIONS)
        if (_linkFlags)
            appendToList(linkFlags1 "${_linkFlags}")
        endif()
        get_target_property(_linkDirs ${lib} INTERFACE_LINK_DIRECTORES)
        if (_linkDirs)
            foreach(flag ${_linkDirs})
                handleGeneratorExprs(flag)
                appendToList(linkFlags1 "-L${flag}")
                appendToList(libDirs1 "${flag}")
            endforeach()
        endif()

    else()
        # link against library with -l option
        handleGeneratorExprs(lib)
        # although technically a linker flag, we put it to lib list because for some linker flags, it is important
        # that they come before libs
        appendToList(linkLibs1 "-l${lib}")
    endif()
    
    set(${linkLibs} "${linkLibs1}" PARENT_SCOPE)
    set(${linkFlags} "${linkFlags1}" PARENT_SCOPE)
    set(${libDirs} "${libDirs1}" PARENT_SCOPE)
    set(${incDirs} "${incDirs1}" PARENT_SCOPE)
    set(${cxxFlags} "${cxxFlags1}" PARENT_SCOPE)
endfunction()


# if we already have cmake-exports for this project:
# sets the vars ${PROJECT_NAME}_INCLUDE_DIRS, ${PROJECT_NAME}_CXX_FLAGS, ${PROJECT_NAME}_LIBRARY_DIRS,
# ${PROJECT_NAME}_LINKER_FLAGS, and ${PROJECT_NAME}_LIBRARIES
# so that compatibility layer is provided automatically.
if(${PROVIDES_EXPORTED_TARGETS})
    #  imported targets should be namespaced, so define namespaced alias
    add_library(ChimeraTK::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

    resolveImportedLib(ChimeraTK::${PROJECT_NAME} linkLibs linkFlags libDirs incDirs cxxFlags)

    # printing results will help resolve problems with auto-generated compatibility layer
    message(VERBOSE "explicitly provided compatibility layer,")
    message(VERBOSE "  old libset: ${${PROJECT_NAME}_LIBRARIES}")
    message(VERBOSE "  old linkflags: ${${PROJECT_NAME}_LINKER_FLAGS}")
    message(VERBOSE "  old cxxflags: ${${PROJECT_NAME}_CXX_FLAGS}")
    message(VERBOSE "  old incDirs: ${${PROJECT_NAME}_INCLUDE_DIRS}")
    message(VERBOSE "  old libDirs: ${${PROJECT_NAME}_LIBRARY_DIRS}")
    set(${PROJECT_NAME}_INCLUDE_DIRS "${incDirs}" )
    set(${PROJECT_NAME}_LIBRARY_DIRS "${libDirs}" )
    set(${PROJECT_NAME}_LIBRARIES "${linkLibs}" )
    set(${PROJECT_NAME}_CXX_FLAGS "${cxxFlags}" )
    set(${PROJECT_NAME}_LINKER_FLAGS "${linkFlags}" )
    message(VERBOSE "will be overwritten by automatically generated compatibility layer from cmake-exports,")
    message(VERBOSE "  new libset: ${${PROJECT_NAME}_LIBRARIES}")
    message(VERBOSE "  new linkflags: ${${PROJECT_NAME}_LINKER_FLAGS}")
    message(VERBOSE "  new cxxflags: ${${PROJECT_NAME}_CXX_FLAGS}")
    message(VERBOSE "  new incDirs: ${${PROJECT_NAME}_INCLUDE_DIRS}")
    message(VERBOSE "  new libDirs: ${${PROJECT_NAME}_LIBRARY_DIRS}")
endif()

# create variables for standard makefiles and pkgconfig
set(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "${${PROJECT_NAME}_CXX_FLAGS}")

string(REPLACE " " ";" LIST "${${PROJECT_NAME}_INCLUDE_DIRS}")
foreach(INCLUDE_DIR ${LIST})
  appendToList(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "-I${INCLUDE_DIR}")
endforeach()

# some old code still might call linker flags _LINK_FLAGS, also include that
appendToList(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINK_FLAGS}")

string(REPLACE " " ";" LIST "${${PROJECT_NAME}_LIBRARY_DIRS}")
foreach(LIBRARY_DIR ${LIST})
  appendToList(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "-L${LIBRARY_DIR}")
endforeach()

if(${PROVIDES_EXPORTED_TARGETS})
    # libraries have already been resolved above, add them to linker flags
    appendToList(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LIBRARIES}")
else()
    # recursive resolution of linker flags is necessary, since dependencies could contain imported targets
    string(REPLACE " " ";" LIST "${PROJECT_NAME} ${${PROJECT_NAME}_LIBRARIES}")
    foreach(LIBRARY ${LIST})
        resolveImportedLib(${LIBRARY} linkLibs linkFlags libDirs incDirs cxxFlags)
        
        appendToList(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "${cxxFlags}")
        
        string(REPLACE " " ";" LIST "${incDirs}")
        foreach(INCLUDE_DIR ${LIST})
            appendToList(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "-I${INCLUDE_DIR}")
        endforeach()
        
        # for some linker flags, it is important that they come before the libs
        prependToList(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${linkFlags}")
        appendToList(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${linkLibs}")
        string(REPLACE " " ";" LIST "${libDirs}")
        foreach(LIBRARY_DIR ${LIST})
            appendToList(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "-L${LIBRARY_DIR}")
        endforeach()
    endforeach()
endif()

set(${PROJECT_NAME}_PUBLIC_DEPENDENCIES_L "")
foreach(DEPENDENCY ${${PROJECT_NAME}_PUBLIC_DEPENDENCIES})
    # we only care about required dependencies: if some lib has an optional dependency and is built against it 
    # after it has been found, the dependency became mandatory for downstream libs.
    # Note, keyword REQUIRED as not according to spec but it works...
    string(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES_L "find_package(${DEPENDENCY} REQUIRED)\n")
endforeach()

if(TARGET ${PROJECT_NAME})
  # set _HAS_LIBRARY only if we have a true library, interface libraries (introduced for imported targets,
  #  e.g. header-only library) don't count.
  get_target_property(targetLoc ${PROJECT_NAME} TYPE)
  if(NOT "INTERFACE_LIBRARY" MATCHES "${targetLoc}")
    set(${PROJECT_NAME}_HAS_LIBRARY 1)
  endif()  
else()
  set(${PROJECT_NAME}_HAS_LIBRARY 0)
endif()


# we have nested @-statements, so we have to parse twice:

# create the cmake find_package configuration file
set(PACKAGE_INIT "@PACKAGE_INIT@") # replacement handled later, so leave untouched here
cmake_policy(SET CMP0053 NEW) # less warnings about irrelevant stuff in comments
configure_file(cmake/PROJECT_NAMEConfig.cmake.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in" @ONLY)
if(${PROVIDES_EXPORTED_TARGETS})
    # we will configure later
else()
    set(PACKAGE_INIT "") # required to avoid parse error
    configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)
endif()
configure_file(cmake/PROJECT_NAMEConfigVersion.cmake.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)

# create the shell script for standard make files
configure_file(cmake/PROJECT_NAME-config.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config" @ONLY)

# create the pkgconfig file
configure_file(cmake/PROJECT_NAME.pc.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}.pc.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}.pc.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" @ONLY)

# install script for Makefiles
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config DESTINATION bin COMPONENT dev)

# install configuration file for pkgconfig
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION share/pkgconfig COMPONENT dev)


if(${PROVIDES_EXPORTED_TARGETS})
    #  imported targets should be namespaced, so define namespaced alias
    add_library(ChimeraTK::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

    # generate and install export file
    install(EXPORT ${PROJECT_NAME}Targets
            FILE ${PROJECT_NAME}Targets.cmake
            NAMESPACE ChimeraTK::
            DESTINATION "lib/cmake/${PROJECT_NAME}"
    )

    include(CMakePackageConfigHelpers)
    # create config file
    # although @ONLY arg is not supported, this behaves in the same way.
    configure_package_config_file("${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
      INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
    )

    # remove any previously installed share/cmake-xx/Modules/Find<ProjectName>.cmake from this project since it does not harmonize with new Config
    set(fileToRemove "share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules/Find${PROJECT_NAME}.cmake")
    install(CODE "FILE(REMOVE ${CMAKE_INSTALL_PREFIX}/${fileToRemove})")
else()
    # install same cmake configuration file another time into the Modules cmake subdirectory for compatibility reasons
    # We do this only if we did not move yet to exported target, since it does not harmonize 
    install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
      DESTINATION share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules RENAME Find${PROJECT_NAME}.cmake COMPONENT dev)

endif()

# install cmake find_package configuration file
install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION "lib/cmake/${PROJECT_NAME}"
        COMPONENT dev
)
