# SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
# SPDX-License-Identifier: LGPL-3.0-or-later

# cmake module for checking the existence python modules
# Usage: find_package(PythonModule COMPONENTS module_a module_b)
#
# returns:
#   PythonModule_FOUND : true if all listed components could be imported as a module
#   PythonModule_<module>_FOUND: true if this particular module was found, false otherwise

# First check for python executable if not provided
if(NOT Python_EXECUTABLE)
    message(STATUS "PythonModule: No python executable found, looking for python 3.0 as baseline")
    find_package(Python 3.0 REQUIRED COMPONENTS Interpreter)
endif()

# Work-around so that we can use find_package_handle_standard_args to do the magic regarding components
set (_PythonModule_Run "yes")

message(STATUS "Checking for requested python modules, using ${Python_EXECUTABLE}")
list(APPEND CMAKE_MESSAGE_INDENT "  ") 
foreach(python_module ${PythonModule_FIND_COMPONENTS})
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import ${python_module}"
        ERROR_VARIABLE DUMMY
        OUTPUT_VARIABLE DUMMY
        RESULT_VARIABLE MODULE_RESULT)
    if(MODULE_RESULT)
        message("\"${python_module}\" could not be found")
    else()
        set(PythonModule_${python_module}_FOUND TRUE)
        message("\"${python_module}\" seems to be available")
    endif()
endforeach()
list(POP_BACK CMAKE_MESSAGE_INDENT)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PythonModule REQUIRED_VARS _PythonModule_Run HANDLE_COMPONENTS)
