#######################################################################################################################
#
# cmake module for finding DOOCS
#
# By default, only the client API is included. If the component "server" is specified, also the
# server library will be used. If the component "zmq" is specified, the DOOCSdzmq library will be used as well.
# Currently support components: api, server, zmq, dapi, ddaq, daqreader, daqsndlib, timinglib
#
# returns:
#   DOOCS_FOUND        : true or false, depending on whether the package was found
#   DOOCS_VERSION      : the package version
#   DOOCS_LIBRARIES    : list of libraries to link against
#   DOOCS_DIR          : doocs library dir
#
# Also returns following for compatibility, however imported targets should be preferred for usage:
#   DOOCS_INCLUDE_DIRS : path to the include directory
#   DOOCS_LIBRARY_DIRS : path to the library directory
#   DOOCS_CXX_FLAGS    : Flags needed to be passed to the c++ compiler
#   DOOCS_LINK_FLAGS   : Flags needed to be passed to the linker
#
# For each component <c>, an imported target DOOCS::<c> is returned.
# We support calling find_package(DOOCS COMPONENTS <cs>) several times, for adding in different components <cs>.
# DOOCS_LIBRARIES will be updated to include all requested components as imported targets.
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

SET(DOOCS_FOUND 0)

# if set, include the --no-as-needed linker flag which helps if inner dependencies between libs are not properly
# set inside the library binaries
set(DOOCS_noAsNeededFlag 1)

# note, helper functions and variables should also be prefixed with DOOCS_, since everything is exported to
#  project calling find_package(DOOCS)

function (DOOCS_prependToPkgConfPath newPath)
    if (NOT (":$ENV{PKG_CONFIG_PATH}:" MATCHES ":${newPath}:"))
        set(ENV{PKG_CONFIG_PATH} ${newPath}:$ENV{PKG_CONFIG_PATH})
    endif()
endfunction()

DOOCS_prependToPkgConfPath(/export/doocs/lib/pkgconfig)
if(DOOCS_DIR)
    # prepend pkgconfig for user-set doocs dir; this makes sure system-installed DOOCS is overwritten
    DOOCS_prependToPkgConfPath(${DOOCS_DIR}/x86_64-linux-gnu/pkgconfig)
    DOOCS_prependToPkgConfPath(${DOOCS_DIR}/pkgconfig)
endif()
if (NOT DOOCS_FIND_QUIETLY)
    message("FindDOOCS: Using PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}")
endif()

# We add the always - required API component
if (NOT (";${DOOCS_FIND_COMPONENTS};" MATCHES ";api;"))
    list(PREPEND DOOCS_FIND_COMPONENTS "api")
endif()

function(expandDoocsComponentName longName shortName)
    if (";${shortName};" MATCHES ";api;")
        set(${longName} "doocs-doocsapi" PARENT_SCOPE)
    elseif (";${shortName};" MATCHES ";zmq;")
        set(${longName} "doocs-doocsdzmq" PARENT_SCOPE)
    elseif (";${shortName};" MATCHES ";dapi;")
        set(${longName} "doocs-doocsdapi" PARENT_SCOPE)
    elseif (";${shortName};" MATCHES ";server;")
        set(${longName} "doocs-serverlib" PARENT_SCOPE)
    elseif (";${shortName};" MATCHES ";ddaq;")
        set(${longName} "doocs-doocsddaq" PARENT_SCOPE)
    elseif (";${shortName};" MATCHES ";daqreader;")
        set(${longName} "doocs-daqreaderlib" PARENT_SCOPE)
    elseif (";${shortName};" MATCHES ";daqsndlib;")
        set(${longName} "doocs-daqsndlib" PARENT_SCOPE)
    elseif (";${shortName};" MATCHES ";timinglib;")
        set(${longName} "doocs-doocstiminglib" PARENT_SCOPE)
    else()
        set(${longName} "${shortName}" PARENT_SCOPE)
    endif()
endfunction()


if(NOT PKG_CONFIG_FOUND)
  include(CMakeFindDependencyMacro)
  find_dependency(PkgConfig)
endif()
# thread libraries are required by DOOCS but seem not to be added through pkgconfig...
find_package(Threads REQUIRED)

# We expect that find_package will be called more than once, with different components.
# Since imported targets cannot be replaced, the only clean solution is to define an imported component per pkgconfig component.
# pkg_check_modules can be called more than once, with different components.
# We define DOOCS_FIND_COMPONENTS_ALL to collect all asked-for components
foreach(component ${DOOCS_FIND_COMPONENTS})
    expandDoocsComponentName(componentLongName ${component})
    if (NOT ";${DOOCS_FIND_COMPONENTS_ALL};" MATCHES ";${componentLongName};")
        list(APPEND DOOCS_FIND_COMPONENTS_ALL ${componentLongName})
        # IMPORTED_TARGET means also imported target PkgConfig::DOOCS will be defined. GLOBAL so we can alias.
        pkg_check_modules(DOOCS_${component} REQUIRED IMPORTED_TARGET GLOBAL ${componentLongName})
        if (DOOCS_${component}_FOUND)
            set(importedTarget PkgConfig::DOOCS_${component})
            if (NOT DOOCS_FIND_QUIETLY)
                message(STATUS "FindDOOCS: imported target is ${importedTarget}. Defining alias DOOCS::${component}")
            endif()
            add_library(DOOCS::${component} ALIAS ${importedTarget})

            set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} "DOOCS::${component}")

            if (${component} STREQUAL "api")
                # add Threads lib only if not yet in
                get_target_property(doocsLinkLibs ${importedTarget} INTERFACE_LINK_LIBRARIES)
                if (NOT (";${doocsLinkLibs};" MATCHES ";Threads::Threads;"))
                    set_target_properties(${importedTarget} PROPERTIES INTERFACE_LINK_LIBRARIES "${doocsLinkLibs};Threads::Threads" )
                endif()
                if(DOOCS_noAsNeededFlag)
                    get_target_property(doocsLinkFlags ${importedTarget} INTERFACE_LINK_OPTIONS)
                    string(REGEX REPLACE ".*-NOTFOUND" "" doocsLinkFlags "${doocsLinkFlags}")
                    set_target_properties(${importedTarget} PROPERTIES INTERFACE_LINK_OPTIONS "-Wl,--no-as-needed;${doocsLinkFlags}")
                endif()
            else()
                # since we did some changes on DOOCS::api, add that as implicit dependency of the other components
                # This makes sure projects not explicitly linking to DOOCS::api have the changes
                get_target_property(doocsLinkLibs ${importedTarget} INTERFACE_LINK_LIBRARIES)
                string(REGEX REPLACE ".*-NOTFOUND" "" doocsLinkLibs "${doocsLinkLibs}")
                set_target_properties(${importedTarget} PROPERTIES INTERFACE_LINK_LIBRARIES "DOOCS::api;${doocsLinkLibs}")
            endif()

            # strip the RPATH from the INTERFACE_LINK_OPTIONS of the imported target
            # cmake will handle the RPATH properly itself when using imported targets. Adding the rpath linker flags
            # explicitly will not only add the directories to the RPATH twice but also destroy the proper ordering,
            # since the directories from the linker flags will be added first and e.g. the build directory will end up
            # too late in the list. As a consequence, test executables might use the installed libraries instead of the
            # library under test from the build directory - even for tests and libraries quite unrelated to DOOCS, as
            # long as they have a DOOCS library as a (direct or indirect) dependency.
            get_target_property(MY_LINK_OPTIONS ${importedTarget} INTERFACE_LINK_OPTIONS)
            string(REGEX REPLACE "-Wl,-rpath,[^;]*" "" MY_LINK_OPTIONS "${MY_LINK_OPTIONS}")
            string(REGEX REPLACE ";;" ";" MY_LINK_OPTIONS "${MY_LINK_OPTIONS}")
            string(REGEX REPLACE "^;" "" MY_LINK_OPTIONS "${MY_LINK_OPTIONS}")
            string(REGEX REPLACE ";$" "" MY_LINK_OPTIONS "${MY_LINK_OPTIONS}")
            set_target_properties(${importedTarget} PROPERTIES INTERFACE_LINK_OPTIONS "${MY_LINK_OPTIONS}")

            # print some info about targets
            get_target_property(doocsIncDirs ${importedTarget} INTERFACE_INCLUDE_DIRECTORIES)
            message(VERBOSE "  include dirs: ${doocsIncDirs}")
            get_target_property(doocsCxxFlags ${importedTarget} INTERFACE_COMPILE_OPTIONS)
            message(VERBOSE "  compile options: ${doocsCxxFlags}")
            get_target_property(doocsLinkFlags ${importedTarget} INTERFACE_LINK_OPTIONS)
            message(VERBOSE "  link options: ${doocsLinkFlags}")
            get_target_property(doocsLinkLibs ${importedTarget} INTERFACE_LINK_LIBRARIES)
            message(VERBOSE "  link libs: ${doocsLinkLibs}")
            get_target_property(doocsLinkDirs ${importedTarget} INTERFACE_LINK_DIRECTORIES)
            message(VERBOSE "  link dirs: ${doocsLinkDirs}")

        else()
            message(FATAL_ERROR "DOOCS component ${component} not found!")
        endif()
    endif()
endforeach()
#message(DEBUG "complete list of searched components: ${DOOCS_FIND_COMPONENTS_ALL}")

# append to list (arg) to space-separated list, only include not yet existing elements
macro(DOOCS_appendListToList list arg)
    foreach(DOOCS_appendListToList_arg ${arg})
        string(FIND " ${${list}} " " ${DOOCS_appendListToList_arg} " DOOCS_appendListToList_pos)
        if (${DOOCS_appendListToList_pos} EQUAL -1)
            string(APPEND ${list} " ${DOOCS_appendListToList_arg}")
            # strip leading spaces since they might cause problems
            string(REGEX REPLACE "^[ \t]+" "" ${list} "${${list}}")
        endif()
    endforeach()
endmacro()

# note, pkg_check_modules output variables <prefix>_VERSION and <prefix>_LIBDIR are different,
# depending on length of given module list!
set(DOOCS_DIR "${DOOCS_api_LIBDIR}")
set(DOOCS_VERSION "${DOOCS_api_VERSION}")

set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# following lines are compatibiliy layer, required only if using project does not make use of imported targets
# here we should gather from all components
set(DOOCS_CFLAGS "")
set(DOOCS_LDFLAGS "")
if(DOOCS_noAsNeededFlag)
    set(DOOCS_LDFLAGS "-Wl,--no-as-needed")
endif()
set(DOOCS_INCLUDE_DIRS "")
set(DOOCS_LIBRARY_DIRS "")
foreach(component api zmq server ddaq daqreader daqsndlib)
    DOOCS_appendListToList(DOOCS_CFLAGS "${DOOCS_${component}_CFLAGS}")
    DOOCS_appendListToList(DOOCS_LDFLAGS "${DOOCS_${component}_LDFLAGS}")
    DOOCS_appendListToList(DOOCS_INCLUDE_DIRS "${DOOCS_${component}_INCLUDE_DIRS}")
    DOOCS_appendListToList(DOOCS_LIBRARY_DIRS "${DOOCS_${component}_LIBRARY_DIRS}")
endforeach()
set(DOOCS_CXX_FLAGS ${DOOCS_CFLAGS})
set(DOOCS_LINKER_FLAGS ${DOOCS_LDFLAGS})
set(DOOCS_LINK_FLAGS ${DOOCS_LINKER_FLAGS})

# use a macro provided by CMake to check if all the listed arguments are valid and set DOOCS_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DOOCS REQUIRED_VARS DOOCS_DIR VERSION_VAR DOOCS_VERSION)
