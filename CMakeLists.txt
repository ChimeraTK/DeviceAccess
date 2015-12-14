cmake_minimum_required (VERSION 2.8.7)

Project(mtca4u-deviceaccess) 

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 03)
set(${PROJECT_NAME}_PATCH_VERSION 02)

set(${PROJECT_NAME}_VERSION
  ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})

Message ( Root )
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set c++ 11 support:
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized")

#use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")

add_definitions(-DDMAP_FILE_PREFIX="${CMAKE_INSTALL_PREFIX}")

#use -DSUPPRESS_AUTO_DOC_BUILD=true to suppress to create the doc with every
#build. The 'make doc' target will still exist
if(SUPPRESS_AUTO_DOC_BUILD)
    unset(DOC_DEPENDENCY)
else(SUPPRESS_AUTO_DOC_BUILD)
    set(DOC_DEPENDENCY ALL)
endif(SUPPRESS_AUTO_DOC_BUILD)

FIND_PACKAGE(Boost COMPONENTS system REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/exception/include)
include_directories("${PROJECT_SOURCE_DIR}/util/include")
include_directories("${PROJECT_SOURCE_DIR}/fileparsers/include")
include_directories("${PROJECT_SOURCE_DIR}/device_backends/include")
include_directories("${PROJECT_SOURCE_DIR}/device/include")
# Only these include directories should be copied for examples. 
get_property(inc_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

#Now add other includes
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory ("${PROJECT_SOURCE_DIR}/exception")
add_subdirectory ("${PROJECT_SOURCE_DIR}/util")
add_subdirectory ("${PROJECT_SOURCE_DIR}/fileparsers")
add_subdirectory ("${PROJECT_SOURCE_DIR}/device_backends")
add_subdirectory ("${PROJECT_SOURCE_DIR}/device")

add_library( ${PROJECT_NAME} SHARED ${SOURCE} ${HEADERS} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
link_directories(${Boost_LIBRARY_DIRS})
target_link_libraries( ${PROJECT_NAME}  -Wl,--whole-archive ${libexception} ${libfileparser} ${libutil}  -Wl,--no-whole-archive ${Boost_LIBRARIES} pthread) 

enable_testing()  
add_subdirectory ("${PROJECT_SOURCE_DIR}/tests")

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

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/include/mtca4u
)

foreach(dir ${inc_dirs} )
file(GLOB headerFiles  "${dir}/*.h")
foreach(headerFile ${headerFiles} )
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy ${headerFile} ${PROJECT_BINARY_DIR}/include/mtca4u
)
endforeach(headerFile)
endforeach(dir)

	
include_directories("${PROJECT_BINARY_DIR}/include")

add_subdirectory ("${PROJECT_SOURCE_DIR}/plugin_example")
add_subdirectory ("${PROJECT_SOURCE_DIR}/examples")


#change the install prefix to the source directory in case the user has not specified a destination 
#i. e. CMAKE_INSTALL_PREFIX is not set manually
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "Install directory
                 prefix" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

#Install the library and the executables
install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib
  )

#all include files go into include/PROJECT_NAME
install(DIRECTORY ${PROJECT_BINARY_DIR}/include/ DESTINATION include)

#create the config files by filling the correct variables into the template (*.cmake.in)
#All variables @VARIABLE@ are replaced with the current values

#some variables we need for regular Makefiles, for which we also provide the config
set(${PROJECT_NAME}_CPPFLAGS "-I${CMAKE_INSTALL_PREFIX}/include ")
set(${PROJECT_NAME}_LDFLAGS "-L${CMAKE_INSTALL_PREFIX}/lib -l${PROJECT_NAME} ")
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
  DESTINATION . COMPONENT dev)

install(PROGRAMS "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config"
  DESTINATION bin COMPONENT dev)

include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_target(doc ${DOC_DEPENDENCY}
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc DESTINATION . COMPONENT dev)
else(DOXYGEN_FOUND)
  message("Doxygen not found, documentation cannot be build.")
endif(DOXYGEN_FOUND) 

