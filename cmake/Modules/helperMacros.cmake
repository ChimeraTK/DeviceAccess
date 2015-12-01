
# This is a hack to get escape ${} as the variable expansion in the files copied
# through configure_file. This is handy when copying shell scripts.
# TODO: move this to a better location
set(Dollar "$")


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
# .py
# .dmap
# .map
# .txt
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
        "${source_directory}/*[!~].sh" # <- filter out abc~.sh
        "${source_directory}/*[!~].py" # <- filter out abc~.py
        "${source_directory}/*[!~].dmap" 
        "${source_directory}/*[!~].map" 
        "${source_directory}/*[!~].txt") 
    foreach( file ${list_of_files_to_copy} )
        configure_file( ${file} ${target_directory} copyonly )
    endforeach( file )
ENDMACRO( COPY_SOURCE_TO_TARGET )


MACRO( ADD_SCRIPTS_AS_TESTS list_of_script_files )
    foreach( script_path ${list_of_script_files} )
        get_filename_component(test_name ${script_path} NAME_WE)
        add_test( ${test_name} ${script_path} )
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
                    ERROR_VARIABLE errorVariable
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
  message(STATUS "Html documentation support enabled; use 'make html'") 
  get_filename_component(sphinx_config_parent_dir ${sphinx_build_config_file} PATH)
  add_custom_target(html
                    COMMAND ${SPHINX_BUILD} -c ${sphinx_config_parent_dir} -b html ${location_of_rst_source_files} ${location_of_built_html_files}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating html documentation" VERBATIM)
                       
  set(HTML_TARGET_ADDED TRUE CACHE INTERNAL "Html target has already been configured")
ENDFUNCTION()