# CMakeLists.txt for server modules
cmake_minimum_required(VERSION 3.5.1)

include_directories(include)
aux_source_directory(src library_sources)

message("In subdir: ** ${PROJECT_NAME}")

# Build the modules into a shared library so they are only
# instanciated once, if multiple server instances are run
add_library(${PROJECT_NAME}lib SHARED ${library_sources})
set_target_properties(${PROJECT_NAME}lib PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION}
                                                    SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME}lib ${ChimeraTK-ApplicationCore_LIBRARIES} ${ADAPTER_LIBRARIES})
