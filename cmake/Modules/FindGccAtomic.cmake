# This scripts finds gcc's built-in atomic shared library (libatomic.so).
# It is required to link against this library on gcc when using 16 byte atomics, even when running on x86_64/amd64.

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

  FIND_LIBRARY(GCCLIBATOMIC_LIBRARY NAMES atomic atomic.so.1 libatomic.so.1
    HINTS
    $ENV{HOME}/local/lib64
    $ENV{HOME}/local/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib64
    /opt/local/lib
    /usr/lib64
    /usr/lib
    /lib64
    /lib
  )

else ()

  SET(GCCLIBATOMIC_LIBRARY "")

endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GCCLIBATOMIC DEFAULT_MSG GCCLIBATOMIC_LIBRARY)

