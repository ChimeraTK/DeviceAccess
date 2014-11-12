PROJECT(Examples)
cmake_minimum_required(VERSION 2.8)

FIND_PACKAGE(MtcaMappedDevice 00.12.00 REQUIRED)
INCLUDE_DIRECTORIES(${MtcaMappedDevice_INCLUDE_DIRS})

SET(TARGETS readWrite mapped accessor dmap)

foreach( TARGET ${TARGETS} )
  add_executable(${TARGET}  ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp)
  target_link_libraries(${TARGET} ${MtcaMappedDevice_LIBRARIES})
endforeach( TARGET )