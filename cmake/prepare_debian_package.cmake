#Prepare the debian control files from the template.
#Basically this is setting the correct version number in most of the files

set(${PROJECT_NAME}_DEBPACKAGE_BASENAME libmtca4u-instacosadev)

#The debian version string must not contain ".", so we use "-"
string(REPLACE "." "-" ${PROJECT_NAME}_DEBVERSION ${${PROJECT_NAME}_SOVERSION})

#We also need the so version of deviceaccess. We extract it from the full version
#What the regex does:
# () creates a reference 
# . any character
# .+ multiple characters, at least one
# \. a dot, as \ has to be escaped in cmake it's \\.
# \1 the first reference, \\1 as \ has to be escaped
# First reference: everything up to the first .
# Second reference: everything between the first and second .
string(REGEX REPLACE "(.+)\\.(.+)\\..+" "\\1.\\2" mtca4u-deviceaccess_SOVERSION ${mtca4u-deviceaccess_VERSION})
message("mtca4u-deviceaccess_SOVERSION ${mtca4u-deviceaccess_SOVERSION}")

#Nothing to change, just copy
file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/compat
           ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/rules
     DESTINATION debian_from_template)

file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/source/format
     DESTINATION debian_from_template/source)

#Adapt the file name
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/DEBPACKAGE_BASENAME_DEBVERSION.install.in
               debian_from_template/${${PROJECT_NAME}_DEBPACKAGE_BASENAME}${${PROJECT_NAME}_DEBVERSION}.install)

#Adapt the file name and/or set the version number
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/control.in
               debian_from_template/control @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/copyright.in
               debian_from_template/copyright @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/DEBPACKAGE_BASENAME_DEBVERSION.shlib.in
               debian_from_template/${${PROJECT_NAME}_DEBPACKAGE_BASENAME}${${PROJECT_NAME}_DEBVERSION}.shlib)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/DEBPACKAGE_BASENAME-dev.install.in
               debian_from_template/${${PROJECT_NAME}_DEBPACKAGE_BASENAME}-dev.install)

#Copy and configure the shell script which performs the actual 
#building of the package
configure_file(${CMAKE_SOURCE_DIR}/cmake/make_debian_package.sh.in
               make_debian_package.sh @ONLY)

#A custom target so you can just run make debian_package
#(You could instead run make_debian_package.sh yourself, hm...)
add_custom_target(debian_package ${CMAKE_BINARY_DIR}/make_debian_package.sh
                  COMMENT Building debian package for tag ${${PROJECT_NAME}_VERSION})

#For convenience: Also create an install script for DESY
#The shared library package has the version number in the package name
set(PACKAGE_NAME "${${PROJECT_NAME}_DEBPACKAGE_BASENAME}${${PROJECT_NAME}_DEBVERSION}")
#The development package does not have the version in the name
set(PACKAGE_DEV_NAME "${${PROJECT_NAME}_DEBPACKAGE_BASENAME}-dev")
set(PACKAGE_FILES_WILDCARDS "${PACKAGE_NAME}_*.deb ${PACKAGE_DEV_NAME}_*.deb ${${PROJECT_NAME}_DEBPACKAGE_BASENAME}_*.changes")

configure_file(${CMAKE_SOURCE_DIR}/cmake/install_debian_package_at_DESY.sh.in
               install_debian_package_at_DESY.sh @ONLY)

