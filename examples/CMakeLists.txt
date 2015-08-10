message("binary dir")
message(${PROJECT_BINARY_DIR})
include_directories("${PROJECT_BINARY_DIR}/include")
SET(TARGETS DeviceClient readWrite mapped MuxedAccessor accessor dmap)
#SET(TARGET DeviceClient)
foreach( TARGET ${TARGETS} )
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
add_executable(${TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp)
target_link_libraries( ${TARGET} ${libexample} ${PROJECT_NAME}) 
endforeach( TARGET )




