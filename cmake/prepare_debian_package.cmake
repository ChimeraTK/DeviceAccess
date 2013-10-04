#Prepare the debian control files from the template.
#Basically this is setting the correct version number in most of the files

#The debian version string must not contain ".", so we use "-"
set(MtcaMappedDevice_DEBVERSION ${MtcaMappedDevice_MAJOR_VERSION}-${MtcaMappedDevice_MINOR_VERSION})

#Nothing to change, just copy
file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/compat
           ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/rules
     DESTINATION debian_from_template)

file(COPY ${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/source/format
     DESTINATION debian_from_template/source)

#Adapt the file name
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/mtcamappeddeviceDEBVERSION.install
               debian_from_template/mtcamappeddevice${MtcaMappedDevice_DEBVERSION}.install COPYONLY)

#Adapt the file name and/or set the version number
configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/control.in
               debian_from_template/control @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/copyright.in
               debian_from_template/copyright @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/mtcamappeddeviceDEBVERSION.shlib.in
               debian_from_template/mtcamappeddevice${MtcaMappedDevice_DEBVERSION}.shlib @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/debian_package_templates/dev-mtcamappeddevice.install.in
               debian_from_template/dev-mtcamappeddevice.install @ONLY)

#Copy and configure the shell script which performs the actual 
#building of the package
configure_file(${CMAKE_SOURCE_DIR}/cmake/make_debian_package.sh.in
               make_debian_package.sh @ONLY)

#A custom target so you can just run make debian_package
#(You could instead run make_debian_package.sh yourself, hm...)
add_custom_target(debian_package ${CMAKE_BINARY_DIR}/make_debian_package.sh
                  COMMENT Building debian package for tag ${MtcaMappedDevice_VERSION})

#For convenience: Also create an install script for DESY
#The shared library package has the version number in the package name
set(PACKAGE_NAME "mtcamappeddevice${MtcaMappedDevice_DEBVERSION}")
#The development package does not have the version in the name
set(PACKAGE_DEV_NAME "dev-mtcamappeddevice")
set(PACKAGE_FILES_WILDCARDS "${PACKAGE_NAME}_*.deb ${PACKAGE_DEV_NAME}_*.deb mtcamappeddevice_*.changes")

configure_file(${CMAKE_SOURCE_DIR}/cmake/install_debian_package_at_DESY.sh.in
               install_debian_package_at_DESY.sh @ONLY)

