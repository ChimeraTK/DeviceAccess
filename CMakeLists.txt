cmake_minimum_required(VERSION 3.16)
Project(ChimeraTK-DeviceAccess)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 03)
set(${PROJECT_NAME}_MINOR_VERSION 14)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

# C++ compiler flags needed to compile this project and against this project
# The -D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR flag is only needed for
# libxml++ 2.6.
set(${PROJECT_NAME}_CXX_FLAGS "${${PROJECT_NAME}_CXX_FLAGS} -D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR")

include(cmake/set_default_build_to_release.cmake)

# modifies ${PROJECT_NAME}_CXX_FLAGS
include(cmake/set_default_flags.cmake)
include(cmake/enable_code_coverage_report.cmake)

include(cmake/CheckFileOffsetBits.cmake)
CHECK_FILE_OFFSET_BITS()

# use -DSUPPRESS_AUTO_DOC_BUILD=true to suppress to create the doc with every
# build. The 'make doc' target will still exist
if(SUPPRESS_AUTO_DOC_BUILD)
  unset(DOC_DEPENDENCY)
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/doc) # otherwise make install might fail
else(SUPPRESS_AUTO_DOC_BUILD)
  set(DOC_DEPENDENCY ALL)
endif(SUPPRESS_AUTO_DOC_BUILD)

FIND_PACKAGE(Boost COMPONENTS thread system chrono filesystem REQUIRED)
FIND_PACKAGE(ChimeraTK-cppext 01.05 REQUIRED)
FIND_PACKAGE(exprtk 01.04 REQUIRED)
FIND_PACKAGE(nlohmann_json 03.07 REQUIRED)

FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED IMPORTED_TARGET ${LIBXML++_VERSION})

# The PCIe backend can only be built on Linux, so we define a variable here that
# we can then use in other places.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(HAVE_PCIE_BACKEND 1)
  set(HAVE_XDMA_BACKEND 1)
  set(HAVE_UIO_BACKEND 1)

  # these options are not exported
  add_compile_definitions(CHIMERATK_HAVE_PCIE_BACKEND)
  add_compile_definitions(CHIMERATK_HAVE_XDMA_BACKEND)
  add_compile_definitions(CHIMERATK_HAVE_UIO_BACKEND)
else()
  set(HAVE_PCIE_BACKEND 0)
  set(HAVE_XDMA_BACKEND 0)
  set(HAVE_UIO_BACKEND 0)
endif()

# Generate DeviceAccessVersion.h
configure_file(cmake/DeviceAccessVersion.h.in ${PROJECT_BINARY_DIR}/generated_include/DeviceAccessVersion.h)
include_directories(${PROJECT_BINARY_DIR}/generated_include)
install(DIRECTORY ${PROJECT_BINARY_DIR}/generated_include/ DESTINATION include/ChimeraTK)

# Add source files and include directories
set(subdirs device device_backends exception util fileparsers
  device_backends/NumericAddressedBackend device_backends/DummyBackend device_backends/LogicalNameMapping
  device_backends/Subdevice device_backends/SharedDummy device_backends/Rebot)

if(HAVE_PCIE_BACKEND)
  set(subdirs ${subdirs} device_backends/pcie)
endif()

if(HAVE_XDMA_BACKEND)
  set(subdirs ${subdirs} device_backends/xdma)
endif()

if(HAVE_UIO_BACKEND)
  set(subdirs ${subdirs} device_backends/uio)
endif()

foreach(subdir ${subdirs})
  aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/src sources)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include)
  file(GLOB_RECURSE headers "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include/*")
  set(SOURCE ${SOURCE} ${sources} ${headers})
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include/
    DESTINATION include/ChimeraTK
    PATTERN "internal" EXCLUDE)
endforeach()

include(CheckLibraryExists)

check_library_exists(rt shm_open "sys/mman.h" HAVE_SHMOPEN_RT)

if(HAVE_SHMOPEN_RT)
  set(RT_LIBRARIES "-lrt")
else()
  set(RT_LIBRARIES "")
endif()

# add the library
add_library(${PROJECT_NAME} SHARED ${SOURCE})

# we need the compile options as array to tell cmake they should be exported
include(cmake/format_options.cmake)
formatOptionsAsArray(projectCxxFlags ${${PROJECT_NAME}_CXX_FLAGS})
target_compile_options(${PROJECT_NAME} PUBLIC ${projectCxxFlags})

# exported includes are all under ${CMAKE_INSTALL_PREFIX}/include
target_include_directories(${PROJECT_NAME} INTERFACE "$<INSTALL_INTERFACE:include>")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME}
  PRIVATE ${CMAKE_DL_LIBS} PkgConfig::LibXML++ ChimeraTK::exprtk

  # we set this public because of Backend libs using shm_open (e.g. test code)
  PUBLIC ${RT_LIBRARIES}
  PUBLIC ${Boost_LIBRARIES}

  # cppext is public because anything using DeviceAccess also needs to find cppext headers
  PUBLIC ChimeraTK::ChimeraTK-cppext)

# do not remove runtime paths of the library when installing (helps for unsually located implicit dependencies)
set_property(TARGET ${PROJECT_NAME} PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

# this defines architecture-dependent ${CMAKE_INSTALL_LIBDIR}
include(GNUInstallDirs)

# Generate configuration files for dependencies
set(${PROJECT_NAME}_MEXFLAGS "-I${CMAKE_INSTALL_PREFIX}/include -L${CMAKE_INSTALL_FULL_LIBDIR} -l${PROJECT_NAME}")

# we need the public dependencies so create_cmake_config_files can find them as implicit dependencies
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "Boost COMPONENTS system thread chrono filesystem")
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "ChimeraTK-cppext")

# Install the library and the executables
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# we support our cmake EXPORTS as imported targets
set(PROVIDES_EXPORTED_TARGETS 1)
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)

include(cmake/enable_doxygen_documentation.cmake)

include(cmake/enable_code_style_check.cmake)

# Keep the testing section at the end. It will include boost test, which will modify the Boost_LIBRARIES variable.
# We don't want to link client applications to the boost testing library. This might give trouble.
option(BUILD_TESTS "Build test programs" ON)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()

# Install the tools
foreach(toolScripts chimeratk-lmap-editor)
  install(PROGRAMS "tools/${toolScripts}" DESTINATION bin)
endforeach(toolScripts)

# Install the example source code.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION} COMPONENT doc)
