# Search for the path containing library's headers
find_path( readline_ROOT_DIR NAMES include/readline/readline.h )

# Search for include directory
find_path( readline_INCLUDE_DIR NAMES readline/readline.h
                                HINTS ${readline_ROOT_DIR}/include )

# Search for library
find_library( readline_LIBRARY NAMES readline
                               HINTS ${readline_ROOT_DIR}/lib )

# use a macro provided by CMake to check if all the listed arguments are valid and set readline_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(readline DEFAULT_MSG readline_INCLUDE_DIR readline_LIBRARY )

