cmake_minimum_required (VERSION 3.0.2)
Project(ChimeraTK-DeviceAccess)

set(${PROJECT_NAME}_MAJOR_VERSION 03)
set(${PROJECT_NAME}_MINOR_VERSION 07)
set(${PROJECT_NAME}_PATCH_VERSION 01)
include(cmake/set_version_numbers.cmake)

# C++ compiler flags needed to compile this project and against this project
# The -D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR flag is only needed for
# libxml++ 2.6.
set(${PROJECT_NAME}_CXX_FLAGS "${${PROJECT_NAME}_CXX_FLAGS} -DFUSION_MAX_MAP_SIZE=30 -DFUSION_MAX_VECTOR_SIZE=30 -D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR")

include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)
include(cmake/enable_code_coverage_report.cmake)

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
add_dependency(ChimeraTK-cppext REQUIRED 01.04)
add_dependency(exprtk REQUIRED 01.00)

FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED glib-2.0)
include_directories(SYSTEM ${glib_INCLUDE_DIRS} ${LibXML++_INCLUDE_DIRS})

# The PCIe backend can only be built on Linux, so we define a variable here that
# we can then use in other places.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(HAVE_PCIE_BACKEND 1)
  set(HAVE_XDMA_BACKEND 1)
  set(HAVE_UIO_BACKEND 1)
  add_definitions(-DCHIMERATK_HAVE_PCIE_BACKEND)
  add_definitions(-DCHIMERATK_HAVE_XDMA_BACKEND)
  add_definitions(-DCHIMERATK_HAVE_UIO_BACKEND)
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
  file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include/*.h")
  set(SOURCE ${SOURCE} ${sources} ${headers})
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include/ DESTINATION include/ChimeraTK)
endforeach()

include(CheckLibraryExists)
find_package(Threads)

check_library_exists(rt shm_open "sys/mman.h" HAVE_SHMOPEN_RT)
if(HAVE_SHMOPEN_RT)
    set(RT_LIBRARIES "-lrt")
else()
    set(RT_LIBRARIES "")
endif()

# add the library
add_library( ${PROJECT_NAME} SHARED ${SOURCE} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
link_directories(${Boost_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS} ${LibXML++_LDFLAGS} ${Boost_LIBRARIES} ${LibXML++_LIBRARIES}
                                      ${glib_LIBRARIES} ${RT_LIBRARIES} Threads::Threads ${ChimeraTK-cppext_LIBRARIES})

# Install the library and the executables
install( TARGETS ${PROJECT_NAME}
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib )

# Generate configuration files for dependencies
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include ${Boost_INCLUDE_DIRS} ${ChimeraTK-cppext_INCLUDE_DIRS}")
set(${PROJECT_NAME}_LIBRARIES "${Boost_LIBRARIES} ${ChimeraTK-cppext_LIBRARIES} ${RT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib ${Boost_LIBRARY_DIRS} ${ChimeraTK-cppext_LIBRARY_DIRS}")
# already set: ${PROJECT_NAME}_CXX_FLAGS
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(${PROJECT_NAME}_LINKER_FLAGS "-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib")
else()
  set(${PROJECT_NAME}_LINKER_FLAGS "-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib,--enable-new-dtags")
endif()
set(${PROJECT_NAME}_MEXFLAGS "-I${CMAKE_INSTALL_PREFIX}/include -L${CMAKE_INSTALL_PREFIX}/lib -l${PROJECT_NAME}")

list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "Boost COMPONENTS system thread chrono filesystem")

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
foreach( toolScripts chimeratk-lmap-editor )
  install(PROGRAMS "tools/${toolScripts}" DESTINATION bin)
endforeach( toolScripts )

# Install the example source code.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples DESTINATION share/doc/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION} COMPONENT doc)
