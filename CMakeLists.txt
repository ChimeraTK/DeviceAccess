cmake_minimum_required (VERSION 3.0.2)
Project(ChimeraTK-DeviceAccess)

set(${PROJECT_NAME}_MAJOR_VERSION 01)
set(${PROJECT_NAME}_MINOR_VERSION 00)
set(${PROJECT_NAME}_PATCH_VERSION 02)
include(cmake/set_version_numbers.cmake)

# Set the build type to Release if none is specified
# Force it into Release if "None" is specified (needed to overrule dkpg_buildpackage)
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "None")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# add support for latest C++ standard to compiler flags
include(cmake/enable_latest_cxx_support.cmake)

# C++ compiler flags needed to compile this project and against this project
set(${PROJECT_NAME}_CXX_FLAGS "${${PROJECT_NAME}_CXX_FLAGS} -DFUSION_MAX_MAP_SIZE=30 -DFUSION_MAX_VECTOR_SIZE=30")

# set C++ compiler flags for compiling this project
set(CMAKE_CXX_FLAGS "${${PROJECT_NAME}_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized")

# use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")

# use -DSUPPRESS_AUTO_DOC_BUILD=true to suppress to create the doc with every
# build. The 'make doc' target will still exist
if(SUPPRESS_AUTO_DOC_BUILD)
    unset(DOC_DEPENDENCY)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/doc)  # otherwise make install might fail
else(SUPPRESS_AUTO_DOC_BUILD)
    set(DOC_DEPENDENCY ALL)
endif(SUPPRESS_AUTO_DOC_BUILD)

# add dependencies
include(cmake/add_dependency.cmake)
add_dependency(Boost COMPONENTS thread system chrono filesystem REQUIRED)
add_dependency(ChimeraTK-cppext REQUIRED 01.01)

FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED glib-2.0)
include_directories(SYSTEM ${LibXML++_INCLUDEDIR}/${LIBXML++_VERSION} ${glib_INCLUDE_DIRS} ${LibXML++_INCLUDE_DIRS})

# The PCIe backend can only be built on Linux, so we define a variable here that
# we can then use in other places.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(HAVE_PCIE_BACKEND 1)
  add_definitions(-DCHIMERATK_HAVE_PCIE_BACKEND)
else()
  set(HAVE_PCIE_BACKEND 0)
endif()

# Generate DeviceAccessVersion.h
configure_file(cmake/DeviceAccessVersion.h.in ${PROJECT_BINARY_DIR}/generated_include/DeviceAccessVersion.h)
include_directories(${PROJECT_BINARY_DIR}/generated_include)

# Add source files and include directories
set(subdirs exception util fileparsers device_backends device device_backends/LogicalNameMapping device_backends/Subdevice device_backends/SharedDummy device_backends/Rebot)
if(HAVE_PCIE_BACKEND)
  set(subdirs ${subdirs} device_backends/pcie)
endif()
foreach(subdir ${subdirs})
  aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/src sources)
  set(SOURCE ${SOURCE} ${sources})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include/ DESTINATION include/ChimeraTK)
endforeach()

# add the library
add_library( ${PROJECT_NAME} SHARED ${SOURCE} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
link_directories(${Boost_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME}  ${LibXML++_LDFLAGS} ${Boost_LIBRARIES} ${LibXML++_LIBRARIES} ${glib_LIBRARIES} pthread dl rt)

# The make coverage command is only available in debug mode
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  configure_file(cmake/Makefile.coverage.in ${PROJECT_BINARY_DIR}/Makefile.coverage @ONLY)
  add_custom_target(coverage
    make -f Makefile.coverage
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating test coverage documentation" VERBATIM
  )
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

# Install the library and the executables
install( TARGETS ${PROJECT_NAME}
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib )

# Generate configuration files for dependencies
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include ${Boost_INCLUDE_DIRS}")
set(${PROJECT_NAME}_LIBRARIES "${Boost_LIBRARIES}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib ${Boost_LIBRARY_DIRS}")
# already set: ${PROJECT_NAME}_CXX_FLAGS
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(${PROJECT_NAME}_LINKER_FLAGS "-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib")
else()
  set(${PROJECT_NAME}_LINKER_FLAGS "-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib,--enable-new-dtags")
endif()
set(${PROJECT_NAME}_MEXFLAGS "-I${CMAKE_INSTALL_PREFIX}/include -L${CMAKE_INSTALL_PREFIX}/lib -l${PROJECT_NAME}")
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)

# Doxygen documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_target(doc ${DOC_DEPENDENCY}
                    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  # note the / after ${CMAKE_BINARY_DIR}/doc/. This causes the directory to be renamed to the destination, not copied into
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/ DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION}
          COMPONENT doc)
else()
  message("Doxygen not found, documentation cannot be build.")
endif()

# Keep the testing section at the end. It will include boost test, which will modify the Boost_LIBRARIES variable.
# We don't want to link client applications to the boost testing library. This might give trouble.
enable_testing()
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")

# Install the tools
foreach( toolScripts chimeratk-lmap-editor )
  install(PROGRAMS "${toolScripts}" DESTINATION bin)
endforeach( toolScripts )

# Install the compatibility header files
install(DIRECTORY ${PROJECT_SOURCE_DIR}/mtca4u_compatibility_headers/ DESTINATION include/mtca4u)

# Compatibility with the old project name mtca4u-deviceaccess
configure_file(${PROJECT_SOURCE_DIR}/cmake/Findmtca4u-deviceaccess.cmake.in "${PROJECT_BINARY_DIR}/Findmtca4u-deviceaccess.cmake" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/Findmtca4u-deviceaccess.cmake"
        DESTINATION share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules
        COMPONENT dev)
