MESSAGE(MappedDevice)
MESSAGE (${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE
   ${SOURCE}
   ${CMAKE_CURRENT_SOURCE_DIR}/src/MappedDevice.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ExceptionMappedDevice.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FixedPointConverter.cc
   PARENT_SCOPE
)

set(HEADERS
   ${HEADERS}
   ${CMAKE_CURRENT_SOURCE_DIR}/include/MappedDevice.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/ExceptionMappedDevice.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/FixedPointConverter.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/libdevMap.h
   PARENT_SCOPE
)

