cmake_minimum_required(VERSION 3.10)
project(ChipletPart)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Determine the build type (default to Release if not specified)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Python3 3.10 EXACT REQUIRED COMPONENTS Interpreter Development)
find_package(OpenMP REQUIRED)

add_definitions(-DIDXTYPEWIDTH=64)
add_definitions(-DREALTYPEWIDTH=64)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/graph_part/METIS/include)
include_directories(/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/graph_part/METIS/GKlib)
include_directories(/home/fetzfs_projects/PlacementCluster/envs/cluster/lib/python3.10/site-packages/numpy/core/include)

link_directories(/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/graph_part/METIS/GKlib/build/Linux-x86_64)
link_directories(/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/graph_part/METIS/build/libmetis)

add_executable(chipletPart src/Hypergraph.cpp src/Utilities.cpp src/ChipletPart.cpp src/pugixml.cpp src/floorplan.cpp src/PriorityQueue.cpp src/FMRefiner.cpp src/Main.cpp)

target_include_directories(chipletPart PRIVATE ${Python3_INCLUDE_DIRS})

# Correct use of target_link_libraries with keyword PRIVATE
target_link_libraries(chipletPart PRIVATE Python3::Python Python3::Module pthread metis GKlib)

# Linking OpenMP with keyword PRIVATE
if(OpenMP_CXX_FOUND)
    target_link_libraries(chipletPart PRIVATE OpenMP::OpenMP_CXX)
endif()

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")