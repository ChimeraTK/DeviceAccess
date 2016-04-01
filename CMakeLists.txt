PROJECT(mtca4uInstaCoSADev)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) 

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 00)
set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})

#This library depends on deviceaccess, which is likely to change often.
#We add a build version to the version number to be able to install
#the same software version of the library linked against different versions
#of deviceacces (or other dependencies)
set(${PROJECT_NAME}_BUILDVERSION $ENV{${PROJECT_NAME}_BUILDVERSION})
if( ${PROJECT_NAME}_BUILDVERSION )
  set(${PROJECT_NAME}_SOVERSION "${${PROJECT_NAME}_SOVERSION}${${PROJECT_NAME}_BUILDVERSION}")
endif( ${PROJECT_NAME}_BUILDVERSION )
message("${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_SOVERSION}")
#We need the build name to be part of the libraries full version name
set(${PROJECT_NAME}_FULL_LIBRARY_VERSION ${${PROJECT_NAME}_SOVERSION}.${${PROJECT_NAME}_PATCH_VERSION})

#put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
set(mtca4u-deviceaccess_MIN_VERSION 00.09)
FIND_PACKAGE(mtca4u-deviceaccess ${mtca4u-deviceaccess_MIN_VERSION} REQUIRED)
include_directories(SYSTEM ${mtca4u-deviceaccess_INCLUDE_DIRS})

#put the min version of deviceaccess into a variable. We need it for the debian packaging scripts
set(ControlSystemAdapter_MIN_VERSION 00.00)
FIND_PACKAGE(ControlSystemAdapter ${ControlSystemAdapter_MIN_VERSION} REQUIRED)
include_directories(SYSTEM ${ControlSystemAdapter_INCLUDE_DIRS})

#Moderate version of the compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wextra -ansi -Wuninitialized ${mtca4u-deviceaccess_CMAKE_CXX_FLAGS} -O3 -std=c++11")

#use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")

# linker flags
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-dynamic")

#BOOST unit test framework
FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
IF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  set(TESTING_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  message("Boost unit_test_framework not found, disabling testing")
ENDIF(Boost_UNIT_TEST_FRAMEWORK_FOUND)

include_directories(${CMAKE_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/)
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

MACRO( COPY_MAPPING_FILES )
  foreach( FILE_TO_COPY test.xlmap test.dmap )
    configure_file( tests/${FILE_TO_COPY} ${PROJECT_BINARY_DIR}/${FILE_TO_COPY} copyonly)
  endforeach( FILE_TO_COPY )
ENDMACRO( COPY_MAPPING_FILES )

#Create the executables for automated unit testing.
#Currently we assume that they are all tests contained in one file, so
#each file gives a new executable. This section has to be adapted if this should change.
if(TESTING_IS_ENABLED)
#  COPY_MAPPING_FILES()

  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/executables_src testExecutables)
  foreach( testExecutableSrcFile ${testExecutables})
    #NAME_WE means the base name without path and (longest) extension
    get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
    add_executable(${excutableName} ${testExecutableSrcFile})
    target_link_libraries(${excutableName} ${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES} ${Boost_LIBRARIES})
    add_test(${excutableName} ${excutableName})
  endforeach( testExecutableSrcFile )

  #The make coverage command is only available in debug mode
  IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    configure_file(cmake/Makefile.coverage.in
      ${PROJECT_BINARY_DIR}/Makefile.coverage @ONLY)
    add_custom_target(coverage
      make -f Makefile.coverage
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating test coverage documentation" VERBATIM
      )
  ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

endif(TESTING_IS_ENABLED)

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} ${mtca4u-deviceaccess_LIBRARIES})
# do not remove runtime path of the library  when installing
set_property(TARGET ${PROJECT_NAME} PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

#configure a header file which contains the version information for use in C++
#configure_file(cmake/version.h.in "${PROJECT_BINARY_DIR}/version.h" @ONLY)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/ DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION})
else(DOXYGEN_FOUND)
  message("Doxygen not found, documentation will not be build.")
endif(DOXYGEN_FOUND)


#Install the library and the executables
install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib
  )


#all include files go into include/PROJECT_NAME
#The exclusion of ${PROJECT_NAME} prevents the recursive installation of the files just being installed.
#The original headers are in include/*.h, the installed ones in include/PROJECT_NAME/*.h.
install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/mtca4u/InstaCoSADev
          FILES_MATCHING PATTERN "*.h" 
	  PATTERN ".svn" EXCLUDE
	  PATTERN "${PROJECT_NAME}" EXCLUDE
	  )

#create the config files by filling the correct variables into the template (*.cmake.in)
#All variables @VARIABLE@ are replaced with the current values, but they have to be set first....
set(${PROJECT_NAME}_CONFIG_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS})
set(${PROJECT_NAME}_CONFIG_LIBRARIES ${${PROJECT_NAME}_LIBRARIES})
set(${PROJECT_NAME}_CONFIG_LIBRARY_DIRS ${${PROJECT_NAME}_LIBRARY_DIRS})
set(${PROJECT_NAME}_CONFIG_CXX_FLAGS ${mtca4u-deviceaccess_CPPFLAGS})
set(${PROJECT_NAME}_CPPFLAGS ${mtca4u-deviceaccess_CPPFLAGS})
#create the config files by filling the correct variables into the template (*.cmake.in)
#All variables @VARIABLE@ are replaced with the current values

#some variables we need for regular Makefiles, for which we also provide the config
set(${PROJECT_NAME}_CPPFLAGS "${${PROJECT_NAME}_CPPFLAGS} -I${CMAKE_INSTALL_PREFIX}/include ")
set(${PROJECT_NAME}_LDFLAGS "-L${CMAKE_INSTALL_PREFIX}/lib -l${PROJECT_NAME} ")
#MATLABs mex cannot deal with the rpath and C+=11 flags, so we provide special flags
set(${PROJECT_NAME}_MEXFLAGS "-I${CMAKE_INSTALL_PREFIX}/include ${${PROJECT_NAME}_LDFLAGS} ")
#only now we extend the LDFLAGS to its final value, including the runpath
set(${PROJECT_NAME}_LDFLAGS "${${PROJECT_NAME}_LDFLAGS} -Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib,--enable-new-dtags ")

#We have nested @-statements, so we have to parse twice
configure_file(cmake/${PROJECT_NAME}Config.cmake.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)

configure_file(cmake/${PROJECT_NAME}ConfigVersion.cmake.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)
  
configure_file(cmake/PROJECT_NAME-config.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config" @ONLY)

# Install the ..Config.cmake and ..ConfigVersion.cmake
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" 
  "${CMAKE_SOURCE_DIR}/cmake/Find${PROJECT_NAME}.cmake"
  DESTINATION  share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules COMPONENT dev)

install(PROGRAMS "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config"
  DESTINATION bin COMPONENT dev)
# The instructions to create a debian package are loaded from a separate file
include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)
