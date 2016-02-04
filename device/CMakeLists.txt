set(SOURCE
   ${SOURCE}
   ${CMAKE_CURRENT_SOURCE_DIR}/src/Device.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/src/DeviceException.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/RegisterAccessor.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FixedPointConverter.cc
   PARENT_SCOPE
)

set(HEADERS
   ${HEADERS}
   ${CMAKE_CURRENT_SOURCE_DIR}/include/Device.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/DeviceException.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/RegisterAccessor.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/TwoDRegisterAccessorImpl.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/BufferingRegisterAccessor.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/MultiplexedDataAccessor.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/MultiplexedDataAccessorException.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/FixedPointConverter.h
   PARENT_SCOPE
)

