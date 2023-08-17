# This file contains helper macros to compile python bindings with boost python.


# This is a hack to get escape ${} as the variable expansion in the files copied
# through configure_file. This is handy when copying shell scripts.
# TODO: move this to a better location
set(Dollar "$")



# Call this before find_packages any any function from this file
function(initialize )
    if(PYTHON3)
        #
        # Unfortunately need this for cmake version on trusty to find PythonLibs
        # There also seems to be a bug with cmake on trusty where having this causes
        # find_package to always return the python3 libraries even when python 2
        # libraries are specified . For this reason set this variable only on
        # python3 builds.
        set(Python_ADDITIONAL_VERSIONS 3.4 PARENT_SCOPE)
    endif()
endfunction()

# Returns the python interpreter to be used for teh project. This is indicated
# by the user by invoking cmake with the PYTHON3 bool as True: 
# cmake -DPYTHON3=TRUE ../ 
function(get_python_interpreter_string python_interpreter)
  if(PYTHON3)
      find_program(pyint "python3")
  else()
      find_program(pyint "python")
  endif()
  message("Python interpreter: ${pyint}")
  set(${python_interpreter} ${pyint} PARENT_SCOPE)
endfunction()


# Returns version number depending on choice of compilation; 3 for python3 and
# 2.7 otherwise
function(get_desired_python_major_release version_num)
  if(PYTHON3)
    set(${version_num} "3" PARENT_SCOPE)
  else()
    set(${version_num} "2" PARENT_SCOPE)
  endif()
endfunction()

# os_code_name, empty if not found
function(extract_ubuntu_variant os_code_name)
    find_program(lsb_release lsb_release)
    execute_process(COMMAND ${lsb_release} -cs
                    OUTPUT_VARIABLE lsb_release_os_codename
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${os_code_name} ${lsb_release_os_codename} PARENT_SCOPE)
endfunction()

# Return os_release_version, empty if not found
function(get_os_distro_version os_release_version)
    find_program(lsb_release lsb_release)
    execute_process(COMMAND ${lsb_release} -rs
                    OUTPUT_VARIABLE lsb_release_os_version
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${os_release_version} ${lsb_release_os_version} PARENT_SCOPE)
endfunction()

#
# Returns os_vendor as Ubuntu if applicable 
function (get_os_distro_vendor os_vendor)
    find_program(lsb_release lsb_release)
        execute_process(COMMAND ${lsb_release} -is
                    OUTPUT_VARIABLE lsb_vendor_name
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
                    
    set(${os_vendor} ${lsb_vendor_name} PARENT_SCOPE)
endfunction()


function(get_python_module_install_path python_version_string install_path)
    get_os_distro_vendor(os_vendor)
    convert_version_string_to_list(${python_version_string} version_list)
    list(GET version_list 0 major_version)
    list(GET version_list 1 minor_version)
    
    if((("${os_vendor}" STREQUAL "Ubuntu") OR ("${os_vendor}" STREQUAL "Debian")) AND 
    (("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr") OR ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")))

        if(PYTHON3)
            set(${install_path} 
                "lib/python${major_version}/dist-packages" PARENT_SCOPE)
        else()
            set(${install_path} 
                "lib/python${major_version}.${minor_version}/dist-packages" 
                PARENT_SCOPE)
        endif()

    else()
        set(${install_path} 
            "lib/python${major_version}.${minor_version}/site-packages"
            PARENT_SCOPE)
    endif()
    
endfunction()

function (get_boost_python_component_name pythonlib_version boost_version python_component_name numpy_component_name)

    convert_version_string_to_list(${pythonlib_version} version_list)
    list(GET version_list 0 py_major_version)
    list(GET version_list 1 py_minor_version)

    set(${python_component_name} "python${py_major_version}${py_minor_version}" PARENT_SCOPE)
    set(${numpy_component_name} "numpy${py_major_version}${py_minor_version}" PARENT_SCOPE)

endfunction()

function (convert_version_string_to_list version_string version_list)
    string(REPLACE "." ";" tmp ${${version_string}})
    set(${version_list} ${tmp} PARENT_SCOPE)
endfunction()


# the directory parameter takes in a list of directories within the source
# directory. The macro copies supported file extensions from these specified
# directories into subdirectories in the build folder. The subdirectory will be
# created with the same name as the source directory.
# Eg:
# ${CMAKE_SOURCE_DIR}/a/b/source_dir as input creates
# <project_build_dir>/source_dir and will have the supported files from
# "${CMAKE_SOURCE_DIR}//a/b/source_dir copied to <project_build_dir>/source_dir.
# supported file formats are:
# .sh
# .dmap
# .map
MACRO (COPY_CONTENT_TO_BUILD_DIR directories)
  foreach( directory ${directories} )
      COPY_SUPPORTED_CONTENT( "${directory}" )
  endforeach( directory )
ENDMACRO(COPY_CONTENT_TO_BUILD_DIR)



MACRO( COPY_SUPPORTED_CONTENT directory )
  get_filename_component(parent_directory ${directory} NAME) # Kind of a hack
    # as we are actually picking the directory name and not the filename.
    # (because ${directory} contains path to a directory and not a file)
    set(source_directory "${directory}" )
    set(target_directory "${PROJECT_BINARY_DIR}/${parent_directory}")
    file( MAKE_DIRECTORY "${target_directory}" )
    COPY_SOURCE_TO_TARGET( ${source_directory} ${target_directory} )
ENDMACRO( COPY_SUPPORTED_CONTENT )



# The macro picks up only these specified formats from the
# source directory : .dmap, .map, .txt, .py, .sh. New formats formats may be added by 
# modifying the globbing expression
MACRO( COPY_SOURCE_TO_TARGET source_directory target_directory)
    FILE( GLOB list_of_files_to_copy
        "${source_directory}/*[!~].py" # <- filter out abc~.py
        "${source_directory}/*[!~].dmap" 
        "${source_directory}/*[!~].map" )
        
    foreach( file ${list_of_files_to_copy} )
        get_filename_component(file_name ${file} NAME)
        configure_file( "${file}" "${target_directory}/${file_name}" )
    endforeach( file )
ENDMACRO( COPY_SOURCE_TO_TARGET )


MACRO( ADD_SCRIPTS_AS_TESTS list_of_script_files )
    foreach( script_path ${list_of_script_files} )
        get_filename_component(test_name ${script_path} NAME_WE)
        add_test(NAME ${test_name} COMMAND ${python_interpreter} ${script_path} WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
        message("adding test at ${script_path}")
    endforeach( script_path )
ENDMACRO( ADD_SCRIPTS_AS_TESTS )




FUNCTION(CHECK_FOR_SPHINX)
  if(NOT SPHINX_BUILD)
    # Assume there is no good version of sphinx-build on the build host 
    set(SUPPORTED_SPHINX_VERSION_AVAILABLE FALSE CACHE INTERNAL 
        "Variable indicates if a suitable version of sphinx build is available")
      
    # find if the sphinx-build executable is in the system path
    find_program(SPHINX_BUILD sphinx-build)
  
    # set SUPPORTED_SPHINX_VERSION_AVAILABLE to true if it is a suitable version
    if(SPHINX_BUILD)
      CHECK_SPHINX_BUILD_VERSION()
    endif()
  endif()
ENDFUNCTION()


FUNCTION(CHECK_SPHINX_BUILD_VERSION)
  # set SPHINX_SUPPORTED_BUILD_VERSION in cache as TRUE or NOTFOUND
  if(SPHINX_BUILD)
    execute_process(COMMAND "${SPHINX_BUILD}" "--version" 
                    RESULT_VARIABLE resultVariable
                    ERROR_VARIABLE outputOfVersionCommand
                    OUTPUT_VARIABLE outputOfVersionCommand
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    # was the command successful. Seems --version is not supported on v1.1 which
    # comes with ubuntu!
    if(resultVariable STREQUAL "0") # command succesful
      EXTRACTVERSION(${outputOfVersionCommand})
      MESSAGE(STATUS "Found Sphinx Build Version: ${extractedVersionNumber}")
      if(NOT (extractedVersionNumber LESS 1.3.0))
        set(SUPPORTED_SPHINX_VERSION_AVAILABLE TRUE CACHE INTERNAL 
        "Variable indicates if a suitable version of sphinx build is available"
        FORCE) 
      endif()    
    endif()
  endif()
ENDFUNCTION()

FUNCTION(EXTRACTVERSION outputOfVersionCommand)
  # convert to a list
  string(REGEX REPLACE " " ";" outputlist ${outputOfVersionCommand})
  list(GET outputlist -1 tmp) # <- This is brittle TODO move to regex later
  set(extractedVersionNumber ${tmp} PARENT_SCOPE)
endfunction()

FUNCTION(ADD_HTML_DOCUMENTATION_SUPPORT)
  configure_file(${sphinx_build_confg_in} ${sphinx_build_config_file})
  # copy the config file to the build directory
  message(STATUS "Html documentation support enabled; use 'make doc'") 
  get_filename_component(sphinx_config_parent_dir ${sphinx_build_config_file} PATH)
  add_custom_target(doc
                    COMMAND ${SPHINX_BUILD} -c ${sphinx_config_parent_dir} -b html ${location_of_rst_source_files} ${location_of_built_html_files}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating html documentation" VERBATIM)
  #unfortunately sphinx needs to scan the library, so we first have to run the build step before we get proper documentation
  add_dependencies(doc _da_python_bindings)

  set(DOC_TARGET_ADDED TRUE CACHE INTERNAL "Doc target has been configured")
ENDFUNCTION()
