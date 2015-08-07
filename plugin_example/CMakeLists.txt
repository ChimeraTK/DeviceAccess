set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized -fPIC")
include_directories("${PROJECT_BINARY_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src exampleSources)
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(libexample ${PROJECT_NAME}_example "PARENT_SCOPE")
add_library(${PROJECT_NAME}_example SHARED ${exampleSources})
target_link_libraries( ${PROJECT_NAME}_example ${PROJECT_NAME})  

add_custom_command(TARGET ${PROJECT_NAME}_example PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/include/plugin
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${PROJECT_BINARY_DIR}/include/plugin 
   )


