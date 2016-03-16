include_directories("${PROJECT_BINARY_DIR}/include")
SET(TARGETS basic accessor2D)
foreach( TARGET ${TARGETS} )
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
add_executable(${TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp)
target_link_libraries( ${TARGET} ${libexample} ${PROJECT_NAME}) 
add_test(${TARGET} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET})
endforeach( TARGET )

# mapping and test script for the examples
# They go into a the examples test directory
FILE(COPY example.dmap my_device.map DESTINATION ${PROJECT_BINARY_DIR}/examples)

 #also install the example source code 
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION} COMPONENT doc
  FILES_MATCHING PATTERN "*.cpp"  PATTERN  Makefile PATTERN CMakeLists.txt 
  PATTERN *map PATTERN ".svn" EXCLUDE )
 