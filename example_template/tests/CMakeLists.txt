# CMakeLists.txt for test modules
CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)


# Add BOOST dependencies
FIND_PACKAGE(Boost COMPONENTS thread system unit_test_framework REQUIRED)

# The tests also require the server headers (TODO Potentially, module includes are also needed)
include_directories(${CMAKE_SOURCE_DIR}/server/include)

# Add all tests residing in the "tests" directory
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} testExecutables)
foreach( testExecutableSrcFile ${testExecutables})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
  add_executable(${excutableName} ${testExecutableSrcFile})
  target_sources(${excutableName} PRIVATE ${SERVER_SRC_DIR}/Server.cc)
  set_target_properties(${excutableName} 
    PROPERTIES 
    LINK_FLAGS "${CMAKE_LINK_FLAGS}"
  )
  target_link_libraries(${excutableName} ${PROJECT_NAME}lib ${ChimeraTK-ApplicationCore_LIBRARIES} ${Adapter_LIBRARIES} )
  add_test(
    NAME ${excutableName}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${excutableName}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endforeach( testExecutableSrcFile )
