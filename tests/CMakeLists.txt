FIND_PACKAGE(Boost COMPONENTS system unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})
#add check if boost not found

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(helperMacros)


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/plugin_example/include")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src testSources)
add_library( ${PROJECT_NAME}_TEST_LIBRARY ${testSources} )

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})


aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/executables_src testExecutables)


foreach( testExecutableSrcFile ${testExecutables})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
  add_executable(${excutableName} ${testExecutableSrcFile})
  target_link_libraries(${excutableName} ${PROJECT_NAME} ${PROJECT_NAME}_TEST_LIBRARY ${Boost_LIBRARIES}) 
  add_test(${excutableName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${excutableName})
endforeach(testExecutableSrcFile)

#
# Instructions for compiling the rebot dummy server
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/RebotDummyServer srcFilesForDummyServer)
add_executable(RebotDummyServer ${srcFilesForDummyServer})
target_link_libraries(RebotDummyServer ${PROJECT_NAME} ${BOOST_LIBRARIES})

# 
# Introduced directory unitTestsNotUnderCtest; This contains the boost unit
# tests (currently for the rebot backend). The tests in this directory should
# not be added as ctest tests. The idea is to call them indirectly through a
# wrapper shell script that sets up the environment before they can be run. (eg.
# start the rebot dummy server first.) The lines below would create executables
# for the cpp files in this directory, but not add them to ctest
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/unitTestsNotUnderCtest  executables)
foreach( executablesrc ${executables})
  get_filename_component(exename ${executablesrc} NAME_WE)
  add_executable(${exename} ${executablesrc})
  target_link_libraries(${exename} ${PROJECT_NAME} ${PROJECT_NAME}_TEST_LIBRARY ${Boost_LIBRARIES})
endforeach(executablesrc)


#
# copy the scripts directory to the build location:
COPY_CONTENT_TO_BUILD_DIR("${CMAKE_CURRENT_SOURCE_DIR}/scripts")
#
# Add the scripts as ctests.
FILE(GLOB location_of_script_files 
    "${PROJECT_BINARY_DIR}/scripts/*.sh")
ADD_SCRIPTS_AS_TESTS( "${location_of_script_files}" )


MACRO( COPY_MAPPING_FILES )
  foreach( MAPPING_FILE mtcadummy_withoutModules.map mtcadummy.map mtcadummy_bad.map mtcadummy_bad_fxpoint1.map 
  mtcadummy_bad_fxpoint2.map mtcadummy_bad_fxpoint3.map invalid_metadata.map bam_fmc25_r1225.mapp
  MandatoryRegisterfIeldMissing.map IncorrectRegisterWidth.map IncorrectFracBits1.map
  IncorrectFracBits2.map goodMapFile_withoutModules.map goodMapFile.map mixedMapFile.map badMapFile.map
  dummies.dmap dummies.dmapOld invalid.dmap empty.dmap sequences.map invalidSequences.map muxedDataAcessor.map testDummyRegisterAccessors.map)
    configure_file( ${MAPPING_FILE} ${PROJECT_BINARY_DIR}/tests/${MAPPING_FILE} copyonly)
  endforeach( MAPPING_FILE )
  # The valid dmap file has an absolute path which has to be configured by cmake
  # So this is not "copyonly"
  configure_file( valid.dmap.in ${PROJECT_BINARY_DIR}/tests/valid.dmap)
  configure_file( valid.dmap.in ${PROJECT_BINARY_DIR}/tests/dMapDir/valid.dmap)
  
ENDMACRO( COPY_MAPPING_FILES )

MACRO( COPY_SELECTED_DIRECTORIES )
  foreach( DIRECTORY  dMapDir )
    file( COPY ${DIRECTORY} DESTINATION ${PROJECT_BINARY_DIR}/tests )
  endforeach( DIRECTORY )
  
  foreach( DIRECTORY  GoodDmapDir )
    file( COPY ${DIRECTORY} DESTINATION ${PROJECT_BINARY_DIR}/tests )
  endforeach( DIRECTORY )
  
  foreach( DIRECTORY  emptyMapFile )
    file( COPY ${DIRECTORY} DESTINATION ${PROJECT_BINARY_DIR}/tests )
  endforeach( DIRECTORY )

  foreach( DIRECTORY  BlankFiles )
    file( COPY ${DIRECTORY} DESTINATION ${PROJECT_BINARY_DIR}/tests )
  endforeach( DIRECTORY )

  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests/EmptyDir)
	
ENDMACRO( COPY_SELECTED_DIRECTORIES )

COPY_MAPPING_FILES()
COPY_SELECTED_DIRECTORIES()

ADD_CUSTOM_TARGET(COPY_FAKE_DEVICE_REF_FILE ALL
       COMMAND ${CMAKE_COMMAND} -E 
      copy  ${CMAKE_SOURCE_DIR}/tests/._Reference_Device ${PROJECT_BINARY_DIR}/tests/._Reference_Device
      VERBATIM
  )
