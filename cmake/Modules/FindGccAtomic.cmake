# This scripts finds gcc's built-in atomic shared library (libatomic.so).
# It is required to link against this library on gcc when using 16 byte atomics, even when running on x86_64/amd64.

FIND_LIBRARY(GccAtomic_LIBRARY NAMES atomic atomic.so.1 libatomic.so.1
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

# we don't want to export the full path since this introduces problems with yocto cross-compilation
# so replace by simple lib name
if (GccAtomic_LIBRARY)
    set(GccAtomic_LIBRARY "atomic")
endif()
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GccAtomic DEFAULT_MSG GccAtomic_LIBRARY)
