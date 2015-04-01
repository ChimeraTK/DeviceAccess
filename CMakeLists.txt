cmake_minimum_required(VERSION 2.8.0)

project(MtcaMappedDevice)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 14)
set(${PROJECT_NAME}_PATCH_VERSION 00)
set(${PROJECT_NAME}_VERSION
  ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})
 
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -fPIC -Wextra -Wshadow -Weffc++ -pedantic -Wuninitialized")
#skip -Weffc++ for the moment. It gives too many false positives
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized")

#use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")

#use -DSUPPRESS_AUTO_DOC_BUILD=true to suppress to create the doc with every
#build. The 'make doc' target will still exist
if(SUPPRESS_AUTO_DOC_BUILD)
    unset(DOC_DEPENDENCY)
else(SUPPRESS_AUTO_DOC_BUILD)
    set(DOC_DEPENDENCY ALL)
endif(SUPPRESS_AUTO_DOC_BUILD)

#boost as such is required for shared pointers e.g.
FIND_PACKAGE(Boost COMPONENTS REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

#the unit test component is optional
#Some tests need filesystem (which has system as a dependency. Has to be in here for the linker)
FIND_PACKAGE(Boost COMPONENTS system filesystem unit_test_framework)
#No need to check for system, it has to be there as a dependency of filesystem (keep the syntax short)
IF(Boost_UNIT_TEST_FRAMEWORK_FOUND AND Boost_FILESYSTEM_FOUND)
  set(TESTING_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE(Boost_UNIT_TEST_FRAMEWORK_FOUND AND Boost_FILESYSTEM_FOUND)
  message("Boost unit_test_framework or boost filesystem not found, disabling testing")
ENDIF(Boost_UNIT_TEST_FRAMEWORK_FOUND AND Boost_FILESYSTEM_FOUND)

#add the include directory and source directory to the sources
#add the executables to the list of targets
#in case of multiple targets give a semicolon separated list
MACRO( MAPPEDDEVICE_ADDSUBDIR directory executables)
  include_directories(${CMAKE_SOURCE_DIR}/${directory}/include)
  set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/${directory}/include/)
  #all files in the src directory are added to the variable library_sources
  aux_source_directory(${CMAKE_SOURCE_DIR}/${directory}/src library_sources)

  foreach( target ${executables} )
    add_executable(${target} ${CMAKE_SOURCE_DIR}/${directory}/${target}.cpp)
    target_link_libraries(${target} ${PROJECT_NAME})
  endforeach( target )

ENDMACRO(MAPPEDDEVICE_ADDSUBDIR)

MACRO( COPY_MAPPING_FILES )
  foreach( MAPPING_FILE mtcadummy.map mtcadummy.mapp mtcadummy_bad.map mtcadummy_bad_fxpoint1.map 
  mtcadummy_bad_fxpoint2.map mtcadummy_bad_fxpoint3.map invalid_metadata.map
  MandatoryRegisterfIeldMissing.map IncorrectRegisterWidth.map IncorrectFracBits1.map
  IncorrectFracBits2.map goodMapFile.map goodMappFile.mapp mixedMapFile.mapp badMappFile.mapp
  dummies.dmap invalid.dmap empty.dmap)
    configure_file( tests/${MAPPING_FILE} ${PROJECT_BINARY_DIR}/${MAPPING_FILE} copyonly)
  endforeach( MAPPING_FILE )
  # The valid dmap file has an absolute path which has to be configured by cmake
  # So this is not "copyonly"
  configure_file( tests/valid.dmap.in ${PROJECT_BINARY_DIR}/valid.dmap)
  configure_file( tests/valid.dmap.in ${PROJECT_BINARY_DIR}/dMapDir/valid.dmap)

  # mapping and test script for the examples
  # They go into a the examples directory because the dmapFliesParser scans the whole 
  # directory and in the main build directory there are also ivalid ones.
  # We also copy the bad mapping file so we can reuse dummies.dmap.
  foreach( FILE mtcadummy.map mtcadummy.mapp mtcadummy_bad.map dummies.dmap )
    configure_file( tests/${FILE} ${PROJECT_BINARY_DIR}/examples/${FILE} copyonly)
  endforeach( FILE )

ENDMACRO( COPY_MAPPING_FILES )

MACRO( COPY_SELECTED_DIRECTORIES )
  foreach( DIRECTORY  dMapDir )
    file( COPY tests/${DIRECTORY} DESTINATION ${PROJECT_BINARY_DIR}/ )
  endforeach( DIRECTORY )
ENDMACRO( COPY_SELECTED_DIRECTORIES )

MACRO( COPY_DMAP_TEST_DIR )
  set(DMAP_TEST_DIR_1  "dMapDir")
  set(DMAP_TEST_DIR_2  "BlankFiles")
  set(DMAP_TEST_DIR_3  "GoodDmapDir")
  set(DMAP_TEST_DIR_4  "emptyMapFile")
  set(DMAP_TEST_DIR_5  "EmptyDir")
  
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_1} )
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_2} )
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_3} )
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_4} )
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_5} )
  
  foreach( FILE  goodMapFile.map oneDevice.dmap 
  NonUniqueRegName.map NonUniqueCardName.dmap mapFile.map)
    configure_file( tests/${DMAP_TEST_DIR_1}/${FILE} ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_1}/${FILE} copyonly)
  endforeach( FILE )
  
  foreach( FILE  first.dmap second.dmap mapFile1.map mapFile2.map blank)
    configure_file( tests/${DMAP_TEST_DIR_3}/${FILE} ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_3}/${FILE} copyonly)
  endforeach( FILE )
  
  configure_file( tests/${DMAP_TEST_DIR_2}/empty.dmap ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_2}/empty.dmap copyonly)
  configure_file( tests/${DMAP_TEST_DIR_4}/NoMapfile.dmap ${PROJECT_BINARY_DIR}/${DMAP_TEST_DIR_4}/NoMapfile.dmap copyonly)  
ENDMACRO( COPY_DMAP_TEST_DIR )

#Note: The executables are the old, not automated and not boost-based tests.
#They will probably all fail, but still return 0.
MAPPEDDEVICE_ADDSUBDIR( libexbase test-libexbase )
MAPPEDDEVICE_ADDSUBDIR( libdev_v2 test-libdev)
MAPPEDDEVICE_ADDSUBDIR( libmap test-libmap)
MAPPEDDEVICE_ADDSUBDIR( libdevMap test-libdevMap)

#Create the executables for automated unit testing.
#Currently we assume that they are all tests contained in one file, so
#each file gives a new executable. This section has to be adapted if this should change.
if(TESTING_IS_ENABLED)
  include_directories(${CMAKE_SOURCE_DIR}/tests/include)
  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/src testSources)
  add_library(${PROJECT_NAME}_TEST_LIBRARY ${testSources} )

  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/executables_src testExecutables)
  foreach( testExecutableSrcFile ${testExecutables})
    #NAME_WE means the base name without path and (longest) extension
    get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
    add_executable(${excutableName} ${testExecutableSrcFile})
    target_link_libraries(${excutableName} ${PROJECT_NAME} ${PROJECT_NAME}_TEST_LIBRARY ${Boost_LIBRARIES})    
    #target_link_libraries(${excutableName} ${PROJECT_NAME} ${Boost_LIBRARIES})    
    add_test(${excutableName} ${excutableName})
  endforeach( testExecutableSrcFile )

  COPY_MAPPING_FILES()
  COPY_DMAP_TEST_DIR()
  ADD_CUSTOM_TARGET(COPY_FAKE_DEVICE_REF_FILE ALL
       COMMAND ${CMAKE_COMMAND} -E 
      copy  ${CMAKE_SOURCE_DIR}/tests/._Reference_Device ${PROJECT_BINARY_DIR}/._Reference_Device
      VERBATIM
  )
      
  #The make coverage command is only available in debug mode
  IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    configure_file(cmake/Makefile.coverage.in
      ${PROJECT_BINARY_DIR}/Makefile.coverage @ONLY)
    add_custom_target(coverage
      make -f Makefile.coverage
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating test coverage documentation" VERBATIM
      )
  ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

endif(TESTING_IS_ENABLED)

add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})

#change the install prefix to the source directory in case the user has not specified a destination 
#i. e. CMAKE_INSTALL_PREFIX is not set manually
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "Install directory
                 prefix" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

#Install the library and the executables
install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib
  )

#all include files go into include/PROJECT_NAME
install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/${PROJECT_NAME}
          FILES_MATCHING PATTERN "*.h" 
	  PATTERN ".svn" EXCLUDE)

#create the config files by filling the correct variables into the template (*.cmake.in)
#All variables @VARIABLE@ are replaced with the current values

#We have nested @-statements, so we have to parse twice
configure_file(cmake/${PROJECT_NAME}Config.cmake.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)

configure_file(cmake/${PROJECT_NAME}ConfigVersion.cmake.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)
 
# Install the ..Config.cmake and ..ConfigVersion.cmake
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" 
  "${CMAKE_SOURCE_DIR}/cmake/Find${PROJECT_NAME}.cmake"
  DESTINATION . COMPONENT dev)

#Only add the examples if testing is enabled. 
#The binaries are not installed, but tests are run and a coverage report is generated.
if(TESTING_IS_ENABLED)
  #Do not use the example CMakeLists.txt, which is an example how to build against an installed version.
  #Add all files to this build branch
  aux_source_directory(${CMAKE_SOURCE_DIR}/examples exampleExecutables)

  #The examples are made to include from include/MtcaMappedDevice, so we have to trick this
  foreach( includeDir ${${PROJECT_NAME}_INCLUDE_DIRS} )
    file(GLOB headerFiles  "${includeDir}/*.h")
    file(COPY ${headerFiles} DESTINATION ${PROJECT_BINARY_DIR}/include/MtcaMappedDevice)
  endforeach( includeDir )
  INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/include)

  #Now add all the executables
  foreach( exampleExecutableSrcFile ${exampleExecutables})
    #NAME_WE means the base name without path and (longest) extension
    get_filename_component(excutableName ${exampleExecutableSrcFile} NAME_WE)
    add_executable(${excutableName} ${exampleExecutableSrcFile})
    target_link_libraries(${excutableName} ${PROJECT_NAME})
    add_test(NAME Example_${excutableName} WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/examples COMMAND ${excutableName})
  endforeach(exampleExecutableSrcFile)
  
  #also install the example source code 
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples DESTINATION . COMPONENT dev
    FILES_MATCHING PATTERN "*.cpp"  PATTERN  Makefile PATTERN CMakeLists.txt)
endif(TESTING_IS_ENABLED)
 
# The instructions to create a debian package are loaded from a separate file
include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_target(doc ${DOC_DEPENDENCY}
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc DESTINATION . COMPONENT dev)
else(DOXYGEN_FOUND)
  message("Doxygen not found, documentation cannot be build.")
endif(DOXYGEN_FOUND)
