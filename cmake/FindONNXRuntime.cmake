# FindONNXRuntime.cmake
#
# Provides:
#   ONNXRuntime_FOUND
#   ONNXRuntime_INCLUDE_DIRS
#   ONNXRuntime_LIBRARIES
#   ONNXRuntime::onnxruntime (imported target)
#
# Optional hints:
#   -DONNXRuntime_ROOT=/path/to/prefix
#   env ONNXRuntime_ROOT=/path/to/prefix

set(_ONNXRuntime_HINTS "")

if (ONNXRuntime_ROOT)
  list(APPEND _ONNXRuntime_HINTS "${ONNXRuntime_ROOT}")
endif()

if (DEFINED ENV{ONNXRuntime_ROOT})
  list(APPEND _ONNXRuntime_HINTS "$ENV{ONNXRuntime_ROOT}")
endif()

# Common Homebrew prefixes/kegs
list(APPEND _ONNXRuntime_HINTS
  /opt/homebrew/opt/onnxruntime
  /usr/local/opt/onnxruntime
  /opt/homebrew
  /usr/local
)

# Homebrew layout usually has headers under:
#   <prefix>/include/onnxruntime/onnxruntime_cxx_api.h
# Also support upstream layouts:
#   <prefix>/include/onnxruntime_cxx_api.h
#   <prefix>/include/onnxruntime/core/session/onnxruntime_cxx_api.h
find_path(ONNXRuntime_INCLUDE_DIR
  NAMES
    onnxruntime/onnxruntime_cxx_api.h
    onnxruntime_cxx_api.h
    onnxruntime/core/session/onnxruntime_cxx_api.h
  HINTS ${_ONNXRuntime_HINTS}
  PATH_SUFFIXES include
)

find_library(ONNXRuntime_LIBRARY
  NAMES onnxruntime
  HINTS ${_ONNXRuntime_HINTS}
  PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRuntime
  REQUIRED_VARS ONNXRuntime_INCLUDE_DIR ONNXRuntime_LIBRARY
)

if (ONNXRuntime_FOUND)
  set(ONNXRuntime_INCLUDE_DIRS "${ONNXRuntime_INCLUDE_DIR}")
  set(ONNXRuntime_LIBRARIES "${ONNXRuntime_LIBRARY}")

  if (NOT TARGET ONNXRuntime::onnxruntime)
    add_library(ONNXRuntime::onnxruntime UNKNOWN IMPORTED)
    set_target_properties(ONNXRuntime::onnxruntime PROPERTIES
      IMPORTED_LOCATION "${ONNXRuntime_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ONNXRuntime_INCLUDE_DIR}"
    )
  endif()
endif()
