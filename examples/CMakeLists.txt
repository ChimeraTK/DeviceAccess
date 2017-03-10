#This CMakeLists.txt is to build and run the examples as tests when building the
#deviceaccess library, and to install the example with the library.
#It is not part of the example proper.
include_directories("${PROJECT_BINARY_DIR}/include")

FILE(GLOB SOURCE_FILES *.cpp)

foreach( SOURCE_FILE ${SOURCE_FILES} )
	get_filename_component(TARGET ${SOURCE_FILE} NAME_WE)
    link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    add_executable(${TARGET} ${SOURCE_FILE})
    target_link_libraries( ${TARGET} ${libexample} ${PROJECT_NAME}) 
    add_test(${TARGET} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET})
endforeach( SOURCE_FILE )

# mapping and test script for the examples
# They go into a the examples test directory
FILE(COPY example.dmap my_device.map DESTINATION ${PROJECT_BINARY_DIR}/examples)

#Also install the example source code. 
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION} COMPONENT doc
  FILES_MATCHING PATTERN "*.cpp" PATTERN *map PATTERN ".svn" EXCLUDE )
# Rename the Makefile during installation. It is not called just 'Makefile' to avoid conflicts with 
# the Makefile which is creates by CMake when doing an in-source build.
# In the installed example it should be called Makefile. We don't have a CMakeLists.tx example at the moment.
install(FILES Makefile.example DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION}/examples RENAME Makefile )

add_subdirectory ("custom_backend_registration")
