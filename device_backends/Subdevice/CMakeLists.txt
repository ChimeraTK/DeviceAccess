set(INCLUDE_DIRS ${INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src sources)
set(SOURCE ${SOURCE} ${sources} PARENT_SCOPE)

file (GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
set(HEADERS ${HEADERS} ${headers} PARENT_SCOPE)
