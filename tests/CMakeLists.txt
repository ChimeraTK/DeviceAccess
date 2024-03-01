FIND_PACKAGE(Boost COMPONENTS system unit_test_framework filesystem)
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
  if (HAVE_PCIE_BACKEND OR NOT(executableName STREQUAL "testDevice" OR executableName STREQUAL "testMtca4uDeviceAccess" OR executableName STREQUAL "testPcieBackend" OR executableName STREQUAL "testRegisterAccess"))
    add_executable(${executableName} ${testExecutableSrcFile})
    target_link_libraries(${executableName} 
        PRIVATE ${Boost_LIBRARIES} ${PROJECT_NAME} ${PROJECT_NAME}_TEST_LIBRARY)
    add_test(${executableName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${executableName})
  endif()
endforeach(testExecutableSrcFile)


#
# Instructions for compiling the rebot dummy server
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/RebotDummyServer/src srcFilesForDummyServerLib)
add_library(RebotDummyServerLib ${srcFilesForDummyServerLib})
target_link_libraries(RebotDummyServerLib PUBLIC ChimeraTK::ChimeraTK-cppext)
add_executable(RebotDummyServer ${CMAKE_CURRENT_SOURCE_DIR}/RebotDummyServer/main.cpp)
# ATTENTION: Do not link against the boost_unit_test_library! Doing so would require #defining BOOST_TEST_DYN_LINK and some
# other quirks. If not done, strange crashes occur on newer boost/gcc versions!
target_link_libraries(RebotDummyServer PRIVATE RebotDummyServerLib ${PROJECT_NAME})

target_link_libraries(testRebotHeartbeatCount PRIVATE RebotDummyServerLib)
target_link_libraries(testRebotConnectionTimeouts PRIVATE RebotDummyServerLib)


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
  if (HAVE_PCIE_BACKEND OR NOT(exename STREQUAL "testPcieErrorHandling"))
    add_executable(${exename} ${executablesrc})
    target_link_libraries(${exename} PRIVATE ${PROJECT_NAME} ${Boost_LIBRARIES} ${PROJECT_NAME}_TEST_LIBRARY)
  endif()
endforeach(executablesrc)


#
# copy the scripts directory to the build location:
COPY_CONTENT_TO_BUILD_DIR("${CMAKE_CURRENT_SOURCE_DIR}/scripts")
# Add the scripts as ctests.
FILE(GLOB location_of_script_files
    "${PROJECT_BINARY_DIR}/scripts/*.sh")
ADD_SCRIPTS_AS_TESTS( "${location_of_script_files}" )

# Copy manual tests to the build location
COPY_CONTENT_TO_BUILD_DIR("${CMAKE_CURRENT_SOURCE_DIR}/manualTests")

MACRO( COPY_MAPPING_FILES )
  # run_performance_test.sh is not a map file but should be copied also into the tests directory
  FILE( COPY mtcadummy_withoutModules.map mtcadummy.map mtcadummyB.map mtcadummy_bad.map mtcadummy_bad_fxpoint1.map
    mtcadummy_bad_fxpoint2.map mtcadummy_bad_fxpoint3.map invalid_metadata.map
    MandatoryRegisterfIeldMissing.map IncorrectRegisterWidth.map IncorrectFracBits1.map
    IncorrectFracBits2.map goodMapFile_withoutModules.map goodMapFile.map mixedMapFile.map
    dummies.dmap dummies.dmapOld invalid.dmap empty.dmap sequences.map newSequences.mapp invalidSequences.map newInvalidSequences.mapp
    muxedDataAcessor.map newMuxedDummies.dmap
    testDummyRegisterAccessors.map mtcadummy_rebot.map valid.xlmap invalid1.xlmap invalid2.xlmap invalid3.xlmap
    invalid4.xlmap invalid5.xlmap invalid6.xlmap invalid7.xlmap
    invalid8.xlmap invalidStartIndex1.xlmap invalidStartIndex2.xlmap
    invalidDuplicateName.xlmap
    withParams.xlmap is_functional.xlmap logicalnamemap.dmap
    mathPlugin.xlmap mathPlugin-broken.xlmap mathPlugin-broken2.xlmap
    mathPluginWithPushPars.dmap mathPluginWithPushPars.map mathPluginWithPushPars.xlmap
    monostableTriggerPlugin.xlmap
    forceReadOnlyPlugin.xlmap forceReadOnlyPlugin2.xlmap typeHintModifierPlugin.xlmap
    performanceTest.map run_performance_test.sh
    badLoadlib.dmap badLoadlib2.dmap unkownKey.dmap
    subdeviceTest.dmap Subdevice.map SubdeviceMuxedArea.map SubdeviceTarget.map
    subdeviceTestAreaHandshake.dmap
    shareddummyTest.dmap shareddummy.map
    sharedDummyUnified.dmap sharedDummyUnified.map
    registerAccess.map floatRawTest.map test3.map unifiedTest.xlmap
    interruptMapFile.map interruptMapFileWithError1.map interruptMapFileWithError2.map interruptMapFileWithError3.map interruptMapFileWithError4.map
    goodMapFileWithComments.map
    doubleBuffer.map doubleBuffer.xlmap
    uioBackendTest.dmap
    uioBackendTest.mapp
    doubleBufferHW.xlmap doubleBufferHW.map doubleBufferHW.dmap testNestedInterrupts.map
    bitRangeReadPlugin.xlmap
    decoratorTest.map
    testMappedImage.dmap testMappedImage.map
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

####################################
# Libraries for testing the backend plugin mechanism. Each file is converted to an so file

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/BackendTestLibs backendTestLibSources)

foreach( backendTestLibSrcFile ${backendTestLibSources})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(libName ${backendTestLibSrcFile} NAME_WE)
  add_library(${libName} SHARED ${backendTestLibSrcFile})
  # for these libraries, cmake EXPORTS are not created anyway, so link PRIVATE
  target_link_libraries(${libName} PRIVATE ${PROJECT_NAME})
endforeach()
