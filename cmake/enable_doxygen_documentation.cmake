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

#######################################################################################################################
# Enable doxygen documentation, mainly (but not only for) for libraries
#
# Requirements:
# * A file 'cmake/Doxyfile.in' which is configured to generate output to ${CMAKE_BINARY_DIR}/doc/
#
# Generates:
# * A target 'doc'
# * The target is automatically build unless -DSUPPRESS_AUTO_DOC_BUILD=true is given at configuration
# * The documentation is installed to share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION}
#
# If generation of the documentation is off, 'make install' might not install documentation, or you install outdated documentation!
#
######################################################################################################################


#use -DSUPPRESS_AUTO_DOC_BUILD=true to suppress to create the doc with every
#build. The 'make doc' target will still exist
if(SUPPRESS_AUTO_DOC_BUILD)
    message("ATTENTION: Automatically building the documentation is disabled. 'make install' might not install documentation, or install outdated documentation! Re-enable by configuring cmake with '-DSUPPRESS_AUTO_DOC_BUILD=false'")
    unset(DOC_DEPENDENCY)
else(SUPPRESS_AUTO_DOC_BUILD)
    message("Automatically building the documentation is enabled. Disable by configuring cmake with '-DSUPPRESS_AUTO_DOC_BUILD=true'")
    set(DOC_DEPENDENCY ALL)
endif(SUPPRESS_AUTO_DOC_BUILD)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  # Add custom version variable for Doxygen since configure_file does not seem to be able to do double dereferencing for ${${PROJECT_NAME}_version} etc.
  set(DOXYGEN_PROJECT_NUMBER ${${PROJECT_NAME}_VERSION})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_target(doc ${DOC_DEPENDENCY}
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  #note the / after ${CMAKE_BINARY_DIR}/doc/. This causes the directory to be renamed to the destination, not copied into
  #The optional allows make install to run even if documentation has not been build.
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/ DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION}
          COMPONENT doc OPTIONAL)
else(DOXYGEN_FOUND)
  message("Doxygen not found, documentation cannot be build.")
endif(DOXYGEN_FOUND) 
