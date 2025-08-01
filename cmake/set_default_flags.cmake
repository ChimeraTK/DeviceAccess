# ######################################################################################################################
# set_default_flags.cmake
#
# Set default compiler flags for C++, including the flags for thelatest C++ standard (see
# enable_latest_cxx_support.cmake)
#
# It will also append ${PROJECT_NAME}_CXX_FLAGS to the CMAKE_CXX_FLAGS, so it is a good idea to set any project
# specific flags before calling this macro.
#
# ######################################################################################################################

# ######################################################################################################################
#
# IMPORTANT NOTE:
#
# DO NOT MODIFY THIS FILE inside a project. Instead update the project-template repository and pull the change from
# there. Make sure to keep the file generic, since it will be used by other projects, too.
#
# If you have modified this file inside a project despite this warning, make sure to cherry-pick all your changes
# into the project-template repository immediately.
#
# ######################################################################################################################

include(cmake/enable_latest_cxx_support.cmake)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;asan;tsan")
# array-bounds and stringop-overflow warnings have many false positives in gcc 13.3 release builds (e.g. with
# nlohman/json)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${${PROJECT_NAME}_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized -Wno-array-bounds -Wno-stringop-overflow")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_TSAN "${CMAKE_CXX_FLAGS} -g -O1 -fsanitize=thread -fno-inline")
set(CMAKE_CXX_FLAGS_ASAN "${CMAKE_CXX_FLAGS} -g -O0 -fsanitize=address -fsanitize=undefined -fsanitize=leak -fno-inline -fno-omit-frame-pointer")

add_compile_definitions("$<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${${PROJECT_NAME}_C_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O3 -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_TSAN "${CMAKE_C_FLAGS} -g -O1 -fsanitize=thread -fno-inline")
set(CMAKE_C_FLAGS_ASAN "${CMAKE_C_FLAGS} -g -O0 -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=leak -fno-inline -fno-omit-frame-pointer")

# Make sure any non-standard library path are added in library or executable targets.
# Since this in done only at install time, behavior of unit tests is not affected.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
