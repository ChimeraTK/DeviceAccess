set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized -fPIC")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src utilSources)
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(libutil ${PROJECT_NAME}_util "PARENT_SCOPE")
add_library(${PROJECT_NAME}_util ${utilSources})
target_link_libraries( ${libutil} ${libexception} ) 




