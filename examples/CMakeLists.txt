#Note: This CMakeLists.txt is a fully working example how to use the
# *installed* DeviceAccess library. It is not included from the main CMakeLists.txt
# which is used to build the library.

#Usage:
# * Create a build directory
#   $ mkdir build; cd build
#
# * Run 'cmake' and point it to the example source directory. You might have to
#   specify where to find the DeviceAccess library if it is not in the
#   system configuration, using either CMAKE_PREFIX_PATH or CMAKE_INSTALL_PREFIX.
#   $ cmake WHEREEVER/YOU/SAVED/DeviceAccess/examples -DCMAKE_INSTALL_PREFIX=DeviceAccess_CUSTOM_INSTALLATION_PATH
#  
# * Build the examples by running make.
#   $ make
#
# * You can run the examples directly from the build directory.
#   $ ./basic

cmake_minimum_required(VERSION 3.16)
project(DeviceAccess-example)

# Search for the DeviceAccess library
# this provides ChimeraTK::ChimeraTK-DeviceAccess with compile and link flags
FIND_PACKAGE(ChimeraTK-DeviceAccess 03.07 REQUIRED)

# Get the name of all .cpp files in the directory
FILE(GLOB SOURCE_FILES *.cpp)

enable_testing()

# Create one executable for each source file, with the same base name
foreach( SOURCE_FILE ${SOURCE_FILES} )
    # NAME_WE is the file name without ending
    get_filename_component(TARGET ${SOURCE_FILE} NAME_WE)
    add_executable(${TARGET} ${SOURCE_FILE})
    # Link the DeviceAccess library to the target executable
    target_link_libraries(${TARGET} PRIVATE ChimeraTK::ChimeraTK-DeviceAccess)
    # We add the target as a test. Run them with 'make test'. In a real application you only do this with the tests.
    add_test(${TARGET} ${TARGET})

    # Install the executable when calling 'make install'
    install(TARGETS ${TARGET} DESTINATION bin)
endforeach( SOURCE_FILE )

# To run the examples we need the map files. Copy them to the build folder.
FILE(COPY example.dmap my_device.map DESTINATION ${PROJECT_BINARY_DIR})

