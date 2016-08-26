#Prepare the debian control files from the template.
#Basically this is setting the correct version number in most of the files

#The debian version string must not contain ".", so we use "-"
string(REPLACE "." "-" mtca4u-deviceaccess_DEBVERSION ${mtca4u-deviceaccess_SOVERSION})

#Nothing to change, just copy
file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/compat
          ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/copyright
          ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/rules
     DESTINATION debian_from_template)

file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/source/format
     DESTINATION debian_from_template/source)

#Adapt the file name
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/libmtca4u-deviceaccessDEBVERSION.install
               debian_from_template/libmtca4u-deviceaccess${mtca4u-deviceaccess_DEBVERSION}.install COPYONLY)

#Adapt the file name and/or set the version number
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/control.in
               debian_from_template/control @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/libmtca4u-deviceaccessDEBVERSION.shlib.in
               debian_from_template/libmtca4u-deviceaccess${mtca4u-deviceaccess_DEBVERSION}.shlib @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/libmtca4u-deviceaccess-dev.install.in
               debian_from_template/libmtca4u-deviceaccess-dev.install @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/libmtca4u-deviceaccess-doc.install.in
               debian_from_template/libmtca4u-deviceaccess-doc.install @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/libmtca4u-deviceaccess-doc.doc-base.in
               debian_from_template/libmtca4u-deviceaccess-doc.doc-base @ONLY)

#For convenience: Also create an install script for DESY
#The shared library package has the version number in the package name
set(PACKAGE_BASE_NAME "libmtca4u-deviceaccess")
set(PACKAGE_NAME "${PACKAGE_BASE_NAME}${mtca4u-deviceaccess_DEBVERSION}")
#The development package does not have the version in the name
set(PACKAGE_DEV_NAME "${PACKAGE_BASE_NAME}-dev")
set(PACKAGE_DOC_NAME "${PACKAGE_BASE_NAME}-doc")
set(PACKAGE_FILES_WILDCARDS "${PACKAGE_NAME}_*.deb ${PACKAGE_DEV_NAME}_*.deb  ${PACKAGE_DOC_NAME}_*.deb libmtca4u-deviceaccess_*.changes")
set(PACKAGE_FULL_LIBRARY_VERSION "${${PROJECT_NAME}_FULL_LIBRARY_VERSION}")
set(PACKAGE_TAG_VERSION "${${PROJECT_NAME}_VERSION}")
set(PACKAGE_MESSAGE "Debian package for MTCA4U deviceaccess ${${PROJECT_NAME}_VERSION}")
#Environment variables must not contain hyphens, but the package name does, so we can't just use it.
set(PACKAGE_BUILDVERSION_ENVIRONMENT_VARIABLE_NAME "mtca4u_deviceaccess_BUILDVERSION")
set(PACKAGE_GIT_URI "https://github.com/ChimeraTK/DeviceAccess.git")

configure_file(${CMAKE_SOURCE_DIR}/cmake/install_debian_package_at_DESY.sh.in
               install_debian_package_at_DESY.sh @ONLY)

#Copy and configure the shell script which performs the actual 
#building of the package
configure_file(${CMAKE_SOURCE_DIR}/cmake/make_debian_package.sh.in
               make_debian_package.sh @ONLY)

#A custom target so you can just run make debian_package
#(You could instead run make_debian_package.sh yourself, hm...)
add_custom_target(debian_package ${CMAKE_BINARY_DIR}/make_debian_package.sh
                  COMMENT Building debian package for tag ${mtca4u-deviceaccess_VERSION})
