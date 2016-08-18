#######################################################################################################################
# prepare_debian_package.cmake
#
# Prepares all files needed to build Debian packages for this project (by running "make debian_package").
#
# The script uses a number of common variables provided by the project-template, especially it requires that
# previously the script set_version_numbers.cmake has been included.
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

# Prepare the debian control files from the template.
# Basically this is setting the correct version number in most of the files

# We need the lower-case form of the project name, since Debian expects package names to be all lower-case
string(TOLOWER "${PROJECT_NAME}" project_name)

# Set a number of variables, mainly to avoid having the project name in the variable name (which would require running
# configure_file twice)
set(PACKAGE_BASE_NAME "lib${project_name}")
set(PACKAGE_NAME "${PACKAGE_BASE_NAME}${${PROJECT_NAME}_DEBVERSION}")
set(PACKAGE_DEV_NAME "${PACKAGE_BASE_NAME}-dev")
set(PACKAGE_DOC_NAME "${PACKAGE_BASE_NAME}-doc")
set(PACKAGE_FILES_WILDCARDS "${PACKAGE_NAME}_*.deb ${PACKAGE_DEV_NAME}_*.deb ${PACKAGE_DOC_NAME}_*.deb ${PACKAGE_BASE_NAME}_*.changes")
set(PACKAGE_PATCH_VERSION "${${PROJECT_NAME}_PATCH_VERSION}")
set(PACKAGE_TAG_VERSION "${${PROJECT_NAME}_VERSION}")
set(PACKAGE_MAJOR_VERSION "${${PROJECT_NAME}_MAJOR_VERSION}")
set(PACKAGE_MINOR_VERSION "${${PROJECT_NAME}_MINOR_VERSION}")
set(PACKAGE_MESSAGE "Debian package for the ${PROJECT_NAME} library version ${${PROJECT_NAME}_VERSION}")

# Nothing to change, just copy
file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/compat DESTINATION debian_from_template)
file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/rules DESTINATION debian_from_template)
file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/source/format DESTINATION debian_from_template/source)
file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/${PACKAGE_BASE_NAME}DEBVERSION.install DESTINATION debian_from_template)

# Replace cmake variables only
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/copyright.in
               debian_from_template/copyright @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/dev-${PACKAGE_BASE_NAME}.install.in
               debian_from_template/dev-${PACKAGE_BASE_NAME}.install @ONLY)

# Replace cmake variables. The #DEBVERSION# will be replaced in make_debian_package.sh, so these are still only templates
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/control.template.in
               debian_from_template/control.template @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/${PACKAGE_BASE_NAME}DEBVERSION.shlib.template.in
               debian_from_template/${PACKAGE_BASE_NAME}DEBVERSION.shlib.template @ONLY)

# Copy and configure the shell script which performs the actual building of the package
configure_file(${CMAKE_SOURCE_DIR}/cmake/make_debian_package.sh.in
               make_debian_package.sh @ONLY)

# A custom target so you can just run make debian_package
# (You could instead run make_debian_package.sh yourself, hm...)
add_custom_target(debian_package ${CMAKE_BINARY_DIR}/make_debian_package.sh
                  COMMENT Building debian package for tag ${${PROJECT_NAME}_SOVERSION})

# For convenience: Also create an install script for DESY
# The shared library package has the version number in the package name
configure_file(${CMAKE_SOURCE_DIR}/cmake/install_debian_package_at_DESY_intern.sh.in
               install_debian_package_at_DESY_intern.sh @ONLY)
