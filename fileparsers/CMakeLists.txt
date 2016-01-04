set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized -fPIC")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src fileparsesrc)

add_library(${PROJECT_NAME}_fileparser ${fileparsesrc} )
set(libfileparser ${PROJECT_NAME}_fileparser "PARENT_SCOPE")
