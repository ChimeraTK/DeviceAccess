#Note: This CMakeLists.txt is a fully working example how to use the
# *installed* DeviceAccess library. It is not included from the main CMakeLists.txt
# which is used to build the library.

#Usage:
# * Create a build directory
#   $ mkdir build; cd build
#
# * Run 'cmake' and point it to the example source directory. You might have to
#   specify where to find the DeviceAccess library if it is not in the
#   system configuration, using CMAKE_MODULE_PATH (You have to adapt the CMAKE version).
#   $ cmake WHEREEVER/YOU/SAVED/DeviceAccess/examples -DCMAKE_MODULE_PATH=DeviceAccess_CUSTOM_INSTALLATION_PATH/share/share/cmake-3.5/Modules
#  
# * Build the examples by running make.
#   $ make
#
# * You can run the examples directly from the build directory.
#   $ ./basic

cmake_minimum_required(VERSION 3.5)

# Search for the DeviceAccess library, set the include path and the
# required compiler flags.
FIND_PACKAGE(ChimeraTK-DeviceAccess 01.02 REQUIRED)
include_directories(SYSTEM ${ChimeraTK-DeviceAccess_INCLUDE_DIRS})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ChimeraTK-DeviceAccess_CXX_FLAGS}")

# Get the name of all .cpp files in the directory
FILE(GLOB SOURCE_FILES *.cpp)

enable_testing()

# Create one executable for each source file, with the same base name
foreach( SOURCE_FILE ${SOURCE_FILES} )
    # NAME_WE is the file name without ending
    get_filename_component(TARGET ${SOURCE_FILE} NAME_WE)
    add_executable(${TARGET} ${SOURCE_FILE})
    # Link the DeviceAccess library to the target executable
    target_link_libraries(${TARGET} ${ChimeraTK-DeviceAccess_LIBRARIES})
    # Set all required linker flags (incl. RPath so the libraries are always found)
    set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "${ChimeraTK-DeviceAccess_LINKER_FLAGS}")

    # We add the target as a test. Run them with 'make test'. In a real application you only do this with the tests.
    add_test(${TARGET} ${TARGET})

    # Install the executable when calling 'make install'
    install(TARGETS ${TARGET} DESTINATION bin)
endforeach( SOURCE_FILE )

# To run the examples we need the map files. Copy them to the build folder.
FILE(COPY example.dmap my_device.map DESTINATION ${PROJECT_BINARY_DIR})

#add_subdirectory ("custom_backend_registration")
