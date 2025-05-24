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
# Global compiler flags for better portability on your machine.
# ---------------------------------------------------------------------------
# Use a conservative optimization level and target native architecture.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -march=native")

# ---------------------------------------------------------------------------
# Boost
# ---------------------------------------------------------------------------
find_package(Boost 1.71.0 REQUIRED)
if(Boost_FOUND)
  message(STATUS "Boost found: enabling high-precision math functions")
  include_directories(${Boost_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Boost library not found")
endif()

# ---------------------------------------------------------------------------
# OpenMP
# ---------------------------------------------------------------------------
option(ENABLE_OPENMP "Enable OpenMP for parallel processing" ON)
if(ENABLE_OPENMP)
  message(STATUS "OpenMP support is enabled")
  find_package(OpenMP)
  if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found: enabling parallel processing")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
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
# METIS and GKlib (local installation)
# ---------------------------------------------------------------------------
# Set the METIS root directory based on your local installation.
set(METIS_ROOT "/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/METIS")
# Define GKlib root directory.
set(GKLIB_ROOT "/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/GKlib")

# Include GKlib headers so that types like idx_t and real_t are defined.
include_directories(${GKLIB_ROOT})

if(EXISTS "${METIS_ROOT}/include/metis.h")
    add_definitions(-DIDXTYPEWIDTH=64)
    add_definitions(-DREALTYPEWIDTH=64)
    message(STATUS "Using local METIS from ${METIS_ROOT}")
    include_directories(${METIS_ROOT}/include)
    
    # Build GKlib as a static library.
    file(GLOB GKLIB_SOURCES "${GKLIB_ROOT}/*.c")
    if(GKLIB_SOURCES)
        add_library(gk STATIC ${GKLIB_SOURCES})
        message(STATUS "Built GKlib static library from ${GKLIB_ROOT}")
    else()
        message(FATAL_ERROR "No GKlib source files found in ${GKLIB_ROOT}")
    endif()

    # If METIS has its own CMakeLists.txt, add it as a subdirectory.
    if(EXISTS "${METIS_ROOT}/CMakeLists.txt")
        add_subdirectory(${METIS_ROOT} ${METIS_ROOT}/build EXCLUDE_FROM_ALL)
        set(HAVE_METIS TRUE)
    else()
        # If METIS is already built and available as a library, specify its library directory.
        set(METIS_LIBRARY_DIR "${METIS_ROOT}/lib")
        link_directories(${METIS_LIBRARY_DIR})
        set(HAVE_METIS TRUE)
    endif()
else()
    message(FATAL_ERROR "Local METIS not found in ${METIS_ROOT}/include")
endif()

# ---------------------------------------------------------------------------
# Eigen
# ---------------------------------------------------------------------------
find_package(Eigen3 REQUIRED)
if(Eigen3_FOUND)    
    message(STATUS "Found installed Eigen library at ${Eigen3_INCLUDE_DIRS}")
    include_directories(${Eigen3_INCLUDE_DIRS})
else()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/eigen3/Eigen/Dense")
        message(STATUS "Found local Eigen library in include directory")
        include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/eigen3")
    else()
        message(STATUS "Eigen not found - downloading from GitLab")
        include(FetchContent)
        FetchContent_Declare(
            eigen
            GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
            GIT_TAG 3.4.0
            GIT_SHALLOW ON
        )
        FetchContent_Populate(eigen)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/eigen3")
        file(COPY "${eigen_SOURCE_DIR}/Eigen" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/eigen3")
        file(COPY "${eigen_SOURCE_DIR}/unsupported" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/eigen3")
        message(STATUS "Copied Eigen headers to ${CMAKE_CURRENT_SOURCE_DIR}/include/eigen3")
        include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/eigen3")
    endif()
endif()

# ---------------------------------------------------------------------------
# Pugixml
# ---------------------------------------------------------------------------
add_library(pugixml STATIC ${PROJECT_SOURCE_DIR}/src/pugixml.cpp)
include_directories(${PROJECT_SOURCE_DIR})

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
        src/KLRefiner.h
        src/Console.h
        src/ChipletPart.h
        src/FMRefiner.h
        src/PriorityQueue.h
        src/Utilities.h
        src/Hypergraph.h
        src/floorplan.h
        src/GeneticTechPartitioner.h
        src/CanonicalGA.cpp
        src/CanonicalGA.h
        src/KLRefiner.cpp
        src/KLRefiner.h
        src/omp_utils.h)
        
    target_link_libraries(chipletPart PRIVATE cost_model pugixml ${Boost_LIBRARIES} Eigen3::Eigen)
    
    # Link METIS and GKlib if available and not disabled (on non-Apple platforms)
    if(HAVE_METIS AND (NOT APPLE))
        target_link_libraries(chipletPart PRIVATE metis gk)
    endif()
    
    # Link OpenMP if found.
    if(OpenMP_CXX_FOUND)
        target_link_libraries(chipletPart PRIVATE ${OpenMP_CXX_LIBRARIES})
    endif()
endif()

# ---------------------------------------------------------------------------
# Compiler flags for different build types
# ---------------------------------------------------------------------------
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
