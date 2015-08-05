cmake_minimum_required (VERSION 2.8.0)
Project(MtcaMappedDevice) #need to change the name
set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 17)
set(${PROJECT_NAME} 0)
set(${PROJECT_NAME}_VERSION
  ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})
Message ( Root )
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized")

include_directories("${PROJECT_SOURCE_DIR}/mappeddevice/include")
include_directories("${PROJECT_SOURCE_DIR}/devices/include")
include_directories(${PROJECT_SOURCE_DIR}/exceptionbase/include exception_inc)
include_directories("${PROJECT_SOURCE_DIR}/fileparsers/include")
include_directories("${PROJECT_SOURCE_DIR}/util/include")

add_subdirectory ("${PROJECT_SOURCE_DIR}/exceptionbase")
add_subdirectory ("${PROJECT_SOURCE_DIR}/util")
add_subdirectory ("${PROJECT_SOURCE_DIR}/mappeddevice")
add_subdirectory ("${PROJECT_SOURCE_DIR}/devices")
add_subdirectory ("${PROJECT_SOURCE_DIR}/fileparsers")
#add_subdirectory ("${PROJECT_SOURCE_DIR}/examples")


add_library( ${PROJECT_NAME} SHARED ${SOURCE} ${HEADERS} )

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
target_link_libraries( ${PROJECT_NAME} ${libutil}) 

enable_testing()
add_subdirectory ("${PROJECT_SOURCE_DIR}/tests")

#use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")
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

