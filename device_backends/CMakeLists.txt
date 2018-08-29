add_subdirectory ("${PROJECT_SOURCE_DIR}/device_backends/LogicalNameMapping")
add_subdirectory ("${PROJECT_SOURCE_DIR}/device_backends/Subdevice")
add_subdirectory ("${PROJECT_SOURCE_DIR}/device_backends/SharedDummy")
if(HAVE_PCIE_BACKEND)
  add_subdirectory ("${PROJECT_SOURCE_DIR}/device_backends/pcie")
endif()
add_subdirectory ("${PROJECT_SOURCE_DIR}/device_backends/Rebot")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src sources)
set(SOURCE ${SOURCE} ${sources} PARENT_SCOPE)

file (GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
set(HEADERS ${HEADERS} ${headers} PARENT_SCOPE)

set(INCLUDE_DIRS ${INCLUDE_DIRS} PARENT_SCOPE)
