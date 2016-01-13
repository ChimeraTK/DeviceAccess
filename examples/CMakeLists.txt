include_directories("${PROJECT_BINARY_DIR}/include")
SET(TARGETS DeviceClient readWrite mapped MuxedAccessor accessor dmap)
foreach( TARGET ${TARGETS} )
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
add_executable(${TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.cpp)
target_link_libraries( ${TARGET} ${libexample} ${PROJECT_NAME}) 
add_test(${TARGET} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET})
endforeach( TARGET )

# mapping and test script for the examples
  # They go into a the examples directory because the dmapFliesParser scans the whole 
  # directory and in the main build directory there are also ivalid ones.
  # We also copy the bad mapping file so we can reuse dummies.dmap.
  foreach( FILE goodMapFile.map mtcadummy_withoutModules.map mtcadummy.map mtcadummy_bad.map  muxedDataAcessor.map dummies.dmap mtcadummy_rebot.map)
    configure_file( ${CMAKE_SOURCE_DIR}/tests/${FILE} ${PROJECT_BINARY_DIR}/examples/${FILE} copyonly)
  endforeach( FILE )

 #also install the example source code 
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples DESTINATION . COMPONENT dev
  FILES_MATCHING PATTERN "*.cpp"  PATTERN  Makefile PATTERN CMakeLists.txt PATTERN ".svn" EXCLUDE )
 