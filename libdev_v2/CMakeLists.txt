cmake_minimum_required(VERSION 2.6)

project(libdev)

set(CMAKE_CXX_FLAGS "-Wall  -fPIC -Wextra -Wshadow -Weffc++ -ansi -pedantic -Wuninitialized")

include_directories(${CMAKE_SOURCE_DIR}/include)

#all files in the src directory are added to the variable library_sources
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

#message("library_sources is ${library_sources}")

add_library(dev SHARED ${library_sources} )

add_executable(test-libdev test-libdev.cpp)
target_link_libraries(test-libdev dev)
set_target_properties(test-libdev PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
