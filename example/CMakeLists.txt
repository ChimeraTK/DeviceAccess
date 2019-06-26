# Note: This CMakeLists.txt is a minimal complete example how to use the
# *installed* application core library. It is not included from the main CMakeLists.txt
# which is used to build ApplicationCore itself.
#
# FIXME
# It does not use the project template because it is in a sub-directory and not in
# its own repository, where the project template could be merged. For real applications
# it is recommended to use the macros from project template (for MSK developments it is mandatory!).

CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)

SET(PROJECT_NAME demoApp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized")

FIND_PACKAGE(ChimeraTK-ApplicationCore 01.06 REQUIRED)
include_directories(SYSTEM ${ChimeraTK-ApplicationCore_INCLUDE_DIRS})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ChimeraTK-ApplicationCore_CXX_FLAGS}")

#FIXME: Make the adapter configurable via command line parameter
FIND_PACKAGE(ChimeraTK-ControlSystemAdapter-DoocsAdapter 00.10 REQUIRED)
include_directories(SYSTEM ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_INCLUDE_DIRS})
link_directories(${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LIBRARY_DIRS})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_CXX_FLAGS}")

AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR} demo_sources)

message("flags ${ChimeraTK-ApplicationCore_LINK_FLAGS}")
message("libs ${ChimeraTK-ApplicationCore_LIBRARIES}")

foreach( EXE ${PROJECT_NAME} ${PROJECT_NAME}-xmlGenerator)
  add_executable(${EXE} ${demo_sources})
  set_target_properties(${EXE} PROPERTIES LINK_FLAGS "${ChimeraTK-ApplicationCore_LINK_FLAGS} ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LINK_FLAGS}")
  target_link_libraries(${EXE} ${ChimeraTK-ApplicationCore_LIBRARIES})
endforeach()

set_target_properties( ${PROJECT_NAME}-xmlGenerator PROPERTIES COMPILE_FLAGS "${ChimeraTK-ApplicationCore_CXX_FLAGS} -DGENERATE_XML")

target_link_libraries(${PROJECT_NAME} ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LIBRARIES})

# copy config files
FILE( COPY dummy.dmap DemoDummy.map demoApp.conf
      DESTINATION ${PROJECT_BINARY_DIR})
