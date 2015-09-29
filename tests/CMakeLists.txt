FIND_PACKAGE(Boost COMPONENTS system filesystem unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})
#add check if boost not found

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

MACRO( COPY_MAPPING_FILES )
  foreach( MAPPING_FILE mtcadummy_withoutModules.map mtcadummy.map mtcadummy_bad.map mtcadummy_bad_fxpoint1.map 
  mtcadummy_bad_fxpoint2.map mtcadummy_bad_fxpoint3.map invalid_metadata.map bam_fmc25_r1225.mapp
  MandatoryRegisterfIeldMissing.map IncorrectRegisterWidth.map IncorrectFracBits1.map
  IncorrectFracBits2.map goodMapFile_withoutModules.map goodMapFile.map mixedMapFile.map badMapFile.map
  dummies.dmap invalid.dmap empty.dmap sequences.map invalidSequences.map muxedDataAcessor.map testDummyRegisterAccessors.map)
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
