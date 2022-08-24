#######################################################################################################################
# enable_code_style_check.cmake
#
# Enable automatic check of coding style as part of the tests.
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

enable_testing()
add_test(NAME coding_style COMMAND ${CMAKE_SOURCE_DIR}/cmake/check-coding-style.sh ${CMAKE_BINARY_DIR}
                           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
