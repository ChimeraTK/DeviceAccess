set(CTK_CLANG_TIDY_VERSION "14" CACHE STRING "Version of the clang-tidy binary to use")
set(CTK_ENABLE_TIDY_WHILE_BUILDING OFF CACHE BOOL "Whether to run clang-tidy on every compilation unit")
if (CTK_ENABLE_TIDY_WHILE_BUILDING)
    set(CMAKE_CXX_CLANG_TIDY /usr/bin/clang-tidy-${CTK_CLANG_TIDY_VERSION};-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy)
    set(CMAKE_C_CLANG_TIDY /usr/bin/clang-tidy-${CTK_CLANG_TIDY_VERSION};-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(run-linter COMMAND run-clang-tidy-${CTK_CLANG_TIDY_VERSION} -p ${CMAKE_BINARY_DIR} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(fix-linter-stepwise COMMAND ${CMAKE_SOURCE_DIR}/cmake/fix-linter-for-all.py --tidy=clang-tidy-${CTK_CLANG_TIDY_VERSION} --apply-tool=clang-apply-replacements-${CTK_CLANG_TIDY_VERSION} --exclude="${CMAKE_BINARY_DIR}" ${CMAKE_BINARY_DIR}/compile_commands.json
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(fix-linter COMMAND run-clang-tidy-${CTK_CLANG_TIDY_VERSION} -fix -format -p ${CMAKE_BINARY_DIR} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


