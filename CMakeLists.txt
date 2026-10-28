cmake_minimum_required(VERSION 3.14)
project(ChipletPart VERSION 1.0)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type is Release.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING 
      "Choose the type of build, options are: Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()

# Set output directories.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------------------
# Global compiler flags
# ---------------------------------------------------------------------------
# Optimisation level comes from CMAKE_BUILD_TYPE (Release = -O3 -DNDEBUG).
# -march=native makes the binary non-portable between machines; opt in.
option(ENABLE_NATIVE_ARCH "Compile with -march=native (binary only runs on this CPU family)" OFF)
if(ENABLE_NATIVE_ARCH)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

# ---------------------------------------------------------------------------
# Boost (header-only use: boost/range, boost/algorithm/string)
# ---------------------------------------------------------------------------
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)   # prefer BoostConfig.cmake over FindBoost
endif()
find_package(Boost 1.71.0 REQUIRED)
message(STATUS "Found Boost ${Boost_VERSION}")
include_directories(${Boost_INCLUDE_DIRS})

# `cmake -DBUILD_COST_MODEL_TESTS=ON .. && make && ctest`
enable_testing()

# ---------------------------------------------------------------------------
# OpenMP
# ---------------------------------------------------------------------------
option(ENABLE_OPENMP "Enable OpenMP for parallel processing" ON)
if(ENABLE_OPENMP)
  message(STATUS "OpenMP support is enabled")
  # Apple clang does not ship libomp; give FindOpenMP a hint if Homebrew's libomp is present.
  if(APPLE AND NOT DEFINED OpenMP_ROOT)
    foreach(_omp_prefix "/opt/homebrew/opt/libomp" "/usr/local/opt/libomp")
      if(EXISTS "${_omp_prefix}/include/omp.h")
        set(OpenMP_ROOT "${_omp_prefix}")
        break()
      endif()
    endforeach()
  endif()
  find_package(OpenMP)
  if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found: enabling parallel processing")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    # With Apple clang + Homebrew libomp, omp.h is not on the default include path.
    if(OpenMP_CXX_INCLUDE_DIRS)
      include_directories(${OpenMP_CXX_INCLUDE_DIRS})
    endif()
    add_definitions(-DHAVE_OPENMP=1)
  else()
    message(STATUS "OpenMP not found: parallel processing will be disabled")
    add_definitions(-DHAVE_OPENMP=0)
  endif()
else()
  message(STATUS "OpenMP support is disabled by user configuration")
  add_definitions(-DHAVE_OPENMP=0)
endif()

# ---------------------------------------------------------------------------
# METIS and GKlib
# ---------------------------------------------------------------------------
# Three ways to satisfy the METIS dependency, tried in order:
#   1. A local source/prebuilt tree:  -DMETIS_ROOT=<dir> -DGKLIB_ROOT=<dir>
#      (METIS_ROOT must contain include/metis.h; GKLIB_ROOT must contain the
#      GKlib *.c sources). Defaults to <repo>/METIS and <repo>/GKlib.
#   2. A system / package-manager METIS (e.g. `brew install metis`,
#      `apt install libmetis-dev`). Override with -DMETIS_PREFIX=<prefix>.
#   3. Nothing found -> build with -DDISABLE_METIS; ChipletPart falls back to
#      random initial partitions (see ChipletPart::METISPart fallback).
set(METIS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/METIS" CACHE PATH "Local METIS source/prebuilt root (contains include/metis.h)")
set(GKLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/GKlib" CACHE PATH "Local GKlib source root (contains *.c)")
set(METIS_PREFIX "" CACHE PATH "Install prefix of a system METIS (contains include/metis.h and lib/libmetis.*)")
option(DISABLE_METIS "Build without METIS (random initial partitions instead)" OFF)

set(HAVE_METIS FALSE)
set(METIS_LINK_LIBS "")

if(NOT DISABLE_METIS AND EXISTS "${METIS_ROOT}/include/metis.h")
    # --- Option 1: local tree ------------------------------------------------
    message(STATUS "Using local METIS from ${METIS_ROOT}")
    add_definitions(-DIDXTYPEWIDTH=64)
    add_definitions(-DREALTYPEWIDTH=64)
    include_directories(BEFORE ${METIS_ROOT}/include)
    include_directories(${GKLIB_ROOT})

    file(GLOB GKLIB_SOURCES "${GKLIB_ROOT}/*.c")
    if(GKLIB_SOURCES)
        add_library(gk STATIC ${GKLIB_SOURCES})
        message(STATUS "Built GKlib static library from ${GKLIB_ROOT}")
        list(APPEND METIS_LINK_LIBS gk)
    else()
        message(FATAL_ERROR "No GKlib source files found in ${GKLIB_ROOT}")
    endif()

    if(EXISTS "${METIS_ROOT}/CMakeLists.txt")
        add_subdirectory(${METIS_ROOT} ${METIS_ROOT}/build EXCLUDE_FROM_ALL)
    else()
        link_directories("${METIS_ROOT}/lib")
    endif()
    list(APPEND METIS_LINK_LIBS metis)
    set(HAVE_METIS TRUE)

elseif(NOT DISABLE_METIS)
    # --- Option 2: system METIS ---------------------------------------------
    set(_metis_hints)
    if(METIS_PREFIX)
        list(APPEND _metis_hints "${METIS_PREFIX}")
    endif()
    if(APPLE)
        list(APPEND _metis_hints "/opt/homebrew/opt/metis" "/usr/local/opt/metis")
    endif()
    find_path(METIS_INCLUDE_DIR metis.h
              HINTS ${_metis_hints}
              PATH_SUFFIXES include
              NO_CMAKE_SYSTEM_PATH)
    if(NOT METIS_INCLUDE_DIR)
        find_path(METIS_INCLUDE_DIR metis.h PATH_SUFFIXES include)
    endif()
    find_library(METIS_LIBRARY NAMES metis
                 HINTS ${_metis_hints}
                 PATH_SUFFIXES lib lib64)

    if(METIS_INCLUDE_DIR AND METIS_LIBRARY)
        message(STATUS "Using system METIS: ${METIS_LIBRARY} (headers: ${METIS_INCLUDE_DIR})")
        # Must come BEFORE <repo>/include, which carries a METIS 5.2.1 metis.h
        # whose IDXTYPEWIDTH would not match the installed library.
        include_directories(BEFORE ${METIS_INCLUDE_DIR})
        list(APPEND METIS_LINK_LIBS ${METIS_LIBRARY})
        set(HAVE_METIS TRUE)
    else()
        message(WARNING "METIS not found - building with DISABLE_METIS (random initial partitions). "
                        "Install METIS (e.g. `brew install metis`) or pass -DMETIS_ROOT/-DMETIS_PREFIX.")
        add_definitions(-DDISABLE_METIS)
    endif()
else()
    message(STATUS "METIS disabled by user (DISABLE_METIS=ON)")
    add_definitions(-DDISABLE_METIS)
endif()

# ---------------------------------------------------------------------------
# Eigen (header-only; `brew install eigen` / `apt install libeigen3-dev`)
# ---------------------------------------------------------------------------
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
message(STATUS "Found Eigen ${Eigen3_VERSION}")

# ---------------------------------------------------------------------------
# Pugixml
# ---------------------------------------------------------------------------
add_library(pugixml STATIC ${PROJECT_SOURCE_DIR}/src/pugixml.cpp)
# pugixml.hpp / pugiconfig.hpp live in src/; expose them to every target that links pugixml
# (cost_model headers include "pugixml.hpp" directly).
target_include_directories(pugixml PUBLIC ${PROJECT_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------
# Global include directories
# ---------------------------------------------------------------------------
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/cost_model)
include_directories(${PROJECT_SOURCE_DIR}/cost_model/include)

# ---------------------------------------------------------------------------
# Options for building components
# ---------------------------------------------------------------------------
option(BUILD_COST_MODEL "Build the cost model component" ON)
option(BUILD_MAIN_APP "Build the main ChipletPart application" ON)

if(BUILD_COST_MODEL)
    add_subdirectory(cost_model)
endif()

if(BUILD_MAIN_APP)
    add_executable(chipletPart
        src/Main.cpp
        src/Hypergraph.cpp
        src/Utilities.cpp
        src/ChipletPart.cpp
        src/FMRefiner.cpp
        src/PriorityQueue.cpp
        src/GeneticTechPartitioner.cpp
        src/floorplan.cpp
        src/KLRefiner.cpp
        src/CanonicalGA.cpp)

    target_link_libraries(chipletPart PRIVATE cost_model Eigen3::Eigen)
    
    # Link METIS (and GKlib when built locally) if available.
    if(HAVE_METIS)
        target_link_libraries(chipletPart PRIVATE ${METIS_LINK_LIBS})
    endif()
    
    # Link OpenMP if found.
    if(OpenMP_CXX_FOUND)
        target_link_libraries(chipletPart PRIVATE ${OpenMP_CXX_LIBRARIES})
    endif()
endif()
