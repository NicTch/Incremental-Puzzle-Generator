# FindGUROBI.cmake
# Sets:
#   GUROBI_FOUND
#   GUROBI_HOME            (resolved root)
#   GUROBI_DIR             (= GUROBI_HOME)
#   GUROBI_INCLUDE_DIRS
#   GUROBI_LIBRARY         (C core)
#   GUROBI_CXX_LIBRARY     (C++ lib, Release)
#   GUROBI_CXX_DEBUG_LIBRARY (C++ lib, Debug; falls back to release if missing)

# 1) Resolve GUROBI_HOME
if(GUROBI_HOME)
  message(STATUS "[GUROBI] Using GUROBI_HOME (CMake var): ${GUROBI_HOME}")
elseif(DEFINED ENV{GUROBI_HOME})
  set(GUROBI_HOME "$ENV{GUROBI_HOME}")
  message(STATUS "[GUROBI] Using GUROBI_HOME (env): ${GUROBI_HOME}")
elseif(APPLE)
  # Auto-detect common macOS installs
  file(GLOB _GUROBI_ROOTS "/Library/gurobi*" "/opt/gurobi*" "/usr/local/gurobi*")
  list(SORT _GUROBI_ROOTS)
  list(REVERSE _GUROBI_ROOTS)
  foreach(_root ${_GUROBI_ROOTS})
    if(EXISTS "${_root}/macos_universal2/lib/libgurobi_c++.a")
      set(GUROBI_HOME "${_root}/macos_universal2")
      message(STATUS "[GUROBI] Auto-detected macOS universal2: ${GUROBI_HOME}")
      break()
    elseif(EXISTS "${_root}/mac64/lib/libgurobi_c++.a")
      set(GUROBI_HOME "${_root}/mac64")
      message(STATUS "[GUROBI] Auto-detected macOS mac64: ${GUROBI_HOME}")
      break()
    endif()
  endforeach()
elseif(UNIX)
  # Typical Linux paths (adjust as needed)
  foreach(_p /opt/gurobi /usr/local/gurobi /opt/gurobi100 /opt/gurobi110 /opt/gurobi120)
    if(EXISTS "${_p}/linux64" OR EXISTS "${_p}/lib")
      if(EXISTS "${_p}/linux64/include/gurobi_c.h")
        set(GUROBI_HOME "${_p}/linux64")
      else()
        set(GUROBI_HOME "${_p}")
      endif()
      message(STATUS "[GUROBI] Auto-detected UNIX: ${GUROBI_HOME}")
      break()
    endif()
  endforeach()
endif()

if(NOT GUROBI_HOME)
  message(FATAL_ERROR "[GUROBI] Could not find GUROBI_HOME. Set -DGUROBI_HOME=/path/to/gurobi/<platform>")
endif()

set(GUROBI_DIR "${GUROBI_HOME}")

# 2) Include dir
find_path(GUROBI_INCLUDE_DIRS
  NAMES gurobi_c.h
  PATHS "${GUROBI_HOME}/include"
  PATH_SUFFIXES include
  NO_DEFAULT_PATH
)

if(NOT GUROBI_INCLUDE_DIRS)
  message(FATAL_ERROR "[GUROBI] Could not find gurobi_c.h under ${GUROBI_HOME}/include")
endif()
message(STATUS "[GUROBI] Include: ${GUROBI_INCLUDE_DIRS}")

# 3) Core C library
# Try common versioned names (newest first)
set(_GUROBI_CORE_NAMES gurobi130 gurobi125 gurobi124 gurobi123 gurobi122 gurobi121 gurobi120 gurobi110 gurobi100 gurobi95 gurobi90 gurobi)
find_library(GUROBI_LIBRARY
  NAMES ${_GUROBI_CORE_NAMES}
  PATHS "${GUROBI_HOME}/lib" "${GUROBI_HOME}/lib64" "${GUROBI_HOME}"
  NO_DEFAULT_PATH
)
if(NOT GUROBI_LIBRARY)
  message(FATAL_ERROR "[GUROBI] Could not find Gurobi C library in ${GUROBI_HOME}/lib")
endif()
message(STATUS "[GUROBI] C lib: ${GUROBI_LIBRARY}")

# 4) C++ library (platform-specific)
if(MSVC)
  # ---- Windows (MSVC) ----
  # Determine MD/MT for name
  # Prefer CMake 3.15+ variable; fallback to simple guess
  set(_mflag "md")
  if(CMAKE_MSVC_RUNTIME_LIBRARY)
    if(CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "MultiThreaded")
      set(_mflag "mt")
    endif()
  else()
    # Fallback: if user defines MT=ON, respect it
    if(DEFINED MT AND MT)
      set(_mflag "mt")
    endif()
  endif()

  # VS year suffix (project used 2017); extend if needed
  # Map MSVC_VERSION to year when available; default to 2017 to match your code
  set(_vs_year "2017")

  # Release / Debug candidates
  set(_CXX_RELEASE_NAME "gurobi_c++${_mflag}${_vs_year}")
  set(_CXX_DEBUG_NAME   "gurobi_c++${_mflag}d${_vs_year}")

  find_library(GUROBI_CXX_LIBRARY
    NAMES ${_CXX_RELEASE_NAME} gurobi_c++
    HINTS "${GUROBI_HOME}" "$ENV{GUROBI_HOME}"
    PATHS "${GUROBI_HOME}/lib"
    PATH_SUFFIXES lib
  )
  find_library(GUROBI_CXX_DEBUG_LIBRARY
    NAMES ${_CXX_DEBUG_NAME} ${_CXX_RELEASE_NAME} gurobi_c++
    HINTS "${GUROBI_HOME}" "$ENV{GUROBI_HOME}"
    PATHS "${GUROBI_HOME}/lib"
    PATH_SUFFIXES lib
  )
else()
  # ---- macOS / Unix ----
  # macOS ships a static C++ lib (libgurobi_c++.a) in most installs
  find_library(GUROBI_CXX_LIBRARY
    NAMES gurobi_c++
    PATHS "${GUROBI_HOME}/lib" "${GUROBI_HOME}/lib64" "${GUROBI_HOME}"
    NO_DEFAULT_PATH
  )
  if(NOT GUROBI_CXX_LIBRARY)
    message(FATAL_ERROR "[GUROBI] Could not find Gurobi C++ library in ${GUROBI_HOME}/lib")
  endif()
  # Debug often same as release on mac/unix; reuse if not found separately
  set(GUROBI_CXX_DEBUG_LIBRARY "${GUROBI_CXX_LIBRARY}")
endif()

message(STATUS "[GUROBI] C++ lib (Release): ${GUROBI_CXX_LIBRARY}")
message(STATUS "[GUROBI] C++ lib (Debug):   ${GUROBI_CXX_DEBUG_LIBRARY}")

# 5) Handle result
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GUROBI
  REQUIRED_VARS
    GUROBI_LIBRARY
    GUROBI_CXX_LIBRARY
    GUROBI_INCLUDE_DIRS
)

mark_as_advanced(
  GUROBI_LIBRARY
  GUROBI_CXX_LIBRARY
  GUROBI_CXX_DEBUG_LIBRARY
  GUROBI_INCLUDE_DIRS
  GUROBI_HOME
  GUROBI_DIR
)
