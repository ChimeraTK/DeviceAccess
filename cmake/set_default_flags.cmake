#######################################################################################################################
# set_default_flags.cmake
#
# Set default compiler flags for C++, including the flags for thelatest C++ standard (see
# enable_latest_cxx_support.cmake)
#
# It will also append ${PROJECT_NAME}_CXX_FLAGS to the CMAKE_CXX_FLAGS, so it is a good idea to set any project
# specific flags before calling this macro.
#
#######################################################################################################################

#######################################################################################################################
#
# IMPORTANT NOTE:
#
# DO NOT MODIFY THIS FILE inside a project. Instead update the project-template repository and pull the change from
# there. Make sure to keep the file generic, since it will be used by other projects, too.
#
# If you have modified this file inside a project despite this warning, make sure to cherry-pick all your changes
# into the project-template repository immediately.
#
#######################################################################################################################

include(cmake/enable_latest_cxx_support.cmake)

option(BUILD_COVERAGE_ON_DEBUG "Generate coverage information when compiling in debug mode" ON)

if (BUILD_COVERAGE_ON_DEBUG)
    set(CTK_COVERAGE_FLAGS "--coverage")
else()
    set(CTK_COVERAGE_FLAGS "")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;asan;tsan")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${${PROJECT_NAME}_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 ${CTK_COVERAGE_FLAGS}")
set(CMAKE_CXX_FLAGS_TSAN "${CMAKE_CXX_FLAGS} -g -O1 -fsanitize=thread")
set(CMAKE_CXX_FLAGS_ASAN "${CMAKE_CXX_FLAGS} -g -O0 -fsanitize=address -fsanitize=undefined -fsanitize=leak")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${${PROJECT_NAME}_C_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O3 -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 ${CTK_COVERAGE_FLAGS}")
set(CMAKE_C_FLAGS_TSAN "${CMAKE_C_FLAGS} -g -O1 -fsanitize=thread")
set(CMAKE_C_FLAGS_ASAN "${CMAKE_C_FLAGS} -g -O0 -fsanitize=address -fsanitize=undefined -fsanitize=leak")
