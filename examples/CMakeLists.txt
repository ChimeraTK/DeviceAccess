#PROJECT(Examples)
#cmake_minimum_required(VERSION 2.8)
#FIND_PACKAGE(MtcaMappedDevice 00.12.00 REQUIRED)
#INCLUDE_DIRECTORIES(${MtcaMappedDevice_INCLUDE_DIRS})
#include_directories("/space/nshehzad/work/MtcaMappedDevice/build_ni/include/MtcaMappedDevice")
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
#set(SOURCE
#   ${SOURCE}
#   ${CMAKE_CURRENT_SOURCE_DIR}/ExampleDevice.cpp
#PARENT_SCOPE
#)
#set(HEADERS
#   ${HEADERS}
#   ${CMAKE_CURRENT_SOURCE_DIR}/ExampleDevice.h
#PARENT_SCOPE
#)


SET(TARGET ExampleDevice)
add_executable(${TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp)


