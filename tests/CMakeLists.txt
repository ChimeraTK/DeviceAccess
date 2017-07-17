FIND_PACKAGE(Boost COMPONENTS system unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})
#add check if boost not found

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(helperMacros)


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/RebotDummyServer/include")
include_directories("${PROJECT_SOURCE_DIR}/plugin_example/include")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src testSources)
add_library( ${PROJECT_NAME}_TEST_LIBRARY ${testSources} )

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})


aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/executables_src testExecutables)


foreach( testExecutableSrcFile ${testExecutables})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(executableName ${testExecutableSrcFile} NAME_WE)
  add_executable(${executableName} ${testExecutableSrcFile})
  # ATTENTION: Do not link against the boost_unit_test_library! Doing so would require #defining BOOST_TEST_DYN_LINK and some
  # other quirks. If not done, strange crashes occur on newer boost/gcc versions!
  target_link_libraries(${executableName} ${PROJECT_NAME} ${PROJECT_NAME}_TEST_LIBRARY)
  add_test(${executableName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${executableName})
endforeach(testExecutableSrcFile)

#
# Instructions for compiling the rebot dummy server
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/RebotDummyServer/src srcFilesForDummyServerLib)
add_library(RebotDummyServerLib ${srcFilesForDummyServerLib})
add_executable(RebotDummyServer ${CMAKE_CURRENT_SOURCE_DIR}/RebotDummyServer/main.cpp)
# ATTENTION: Do not link against the boost_unit_test_library! Doing so would require #defining BOOST_TEST_DYN_LINK and some
# other quirks. If not done, strange crashes occur on newer boost/gcc versions!
target_link_libraries(RebotDummyServer RebotDummyServerLib ${PROJECT_NAME})

#target_link_libraries(testRebotHeartbeatCount RebotDummyServerLib)

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
  # ATTENTION: Do not link against the boost_unit_test_library! Doing so would require #defining BOOST_TEST_DYN_LINK and some
  # other quirks. If not done, strange crashes occur on newer boost/gcc versions!
  target_link_libraries(${exename} ${PROJECT_NAME} ${PROJECT_NAME}_TEST_LIBRARY)
endforeach(executablesrc)


#
# copy the scripts directory to the build location:
COPY_CONTENT_TO_BUILD_DIR("${CMAKE_CURRENT_SOURCE_DIR}/scripts")
# This file needs to be prepared with some constants by cmake first
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/testVirtualFunctionTemplateCompilerFlags.sh.in
  ${PROJECT_BINARY_DIR}/scripts/testVirtualFunctionTemplateCompilerFlags.sh)
#
# Add the scripts as ctests.
FILE(GLOB location_of_script_files 
    "${PROJECT_BINARY_DIR}/scripts/*.sh")
ADD_SCRIPTS_AS_TESTS( "${location_of_script_files}" )


MACRO( COPY_MAPPING_FILES )
  # run_performance_test.sh is not a map file but should be copied also into the tests directory
  FILE( COPY mtcadummy_withoutModules.map mtcadummy.map mtcadummy_bad.map mtcadummy_bad_fxpoint1.map 
    mtcadummy_bad_fxpoint2.map mtcadummy_bad_fxpoint3.map invalid_metadata.map bam_fmc25_r1225.mapp
    MandatoryRegisterfIeldMissing.map IncorrectRegisterWidth.map IncorrectFracBits1.map
    IncorrectFracBits2.map goodMapFile_withoutModules.map goodMapFile.map mixedMapFile.map badMapFile.map
    dummies.dmap dummies.dmapOld invalid.dmap empty.dmap sequences.map invalidSequences.map muxedDataAcessor.map
    testDummyRegisterAccessors.map mtcadummy_rebot.map valid.xlmap invalid1.xlmap invalid2.xlmap invalid3.xlmap
    invalid4.xlmap invalid5.xlmap invalid6.xlmap invalid7.xlmap invalid8.xlmap logicalnamemap.dmap 
    performanceTest.map run_performance_test.sh
    badLoadlib.dmap badLoadlib2.dmap unkownKey.dmap
    DESTINATION ${PROJECT_BINARY_DIR}/tests)
  # The valid dmap file has an absolute path which has to be configured by cmake
  # They cannot just be copied.
  configure_file( valid.dmap.in ${PROJECT_BINARY_DIR}/tests/valid.dmap)
  configure_file( valid.dmap.in ${PROJECT_BINARY_DIR}/tests/dMapDir/valid.dmap)
  
  configure_file( runtimeLoading.dmap.in ${PROJECT_BINARY_DIR}/tests/runtimeLoading/runtimeLoading.dmap)
  FILE( COPY goodMapFile.map wrongVersionPlugin.dmap DESTINATION ${PROJECT_BINARY_DIR}/tests/runtimeLoading)
  
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

####################################
# Libraries for testing the backend plugin mechanism. Each file is converted to an so file

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/BackendTestLibs backendTestLibSources)

foreach( backendTestLibSrcFile ${backendTestLibSources})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(libName ${backendTestLibSrcFile} NAME_WE)
  add_library(${libName} SHARED ${backendTestLibSrcFile})
endforeach()
