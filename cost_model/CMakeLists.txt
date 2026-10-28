# CMakeLists.txt for cost_model module
# Included from the top-level CMakeLists.txt (which provides Boost, OpenMP,
# pugixml and the HAVE_OPENMP definition).

# Core design classes (Chip, Layer, WaferProcess, IO, Assembly, Test, ...)
file(GLOB DESIGN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/design/*.cpp")
add_library(Design STATIC ${DESIGN_SOURCES})
target_include_directories(Design PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(Design PUBLIC pugixml)

# XML -> design object construction
add_library(ConstructChip STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/ConstructChip.cpp)
target_link_libraries(ConstructChip PUBLIC Design)

add_library(ReadDesignFromFile STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/ReadDesignFromFile.cpp)
target_link_libraries(ReadDesignFromFile PUBLIC Design)

# Evaluator used by the partitioner
add_library(cost_model STATIC ${CMAKE_CURRENT_SOURCE_DIR}/evaluator_cpp.cpp)
target_include_directories(cost_model PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(cost_model PUBLIC Design ConstructChip ReadDesignFromFile pugixml)
set_target_properties(cost_model PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(OpenMP_CXX_FOUND)
    foreach(_t Design ConstructChip ReadDesignFromFile cost_model)
        target_link_libraries(${_t} PUBLIC ${OpenMP_CXX_LIBRARIES})
    endforeach()
endif()

# Examples (off by default)
option(BUILD_COST_MODEL_EXAMPLES "Build cost model examples" OFF)
if(BUILD_COST_MODEL_EXAMPLES)
    add_executable(construction_example examples/construction_example.cpp)
    target_link_libraries(construction_example ConstructChip)
    add_executable(read_design_example examples/read_design_example.cpp)
    target_link_libraries(read_design_example ReadDesignFromFile)
    file(GLOB DESIGN_EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/examples/design/*.cpp")
    foreach(_ex ${DESIGN_EXAMPLES})
        get_filename_component(_name ${_ex} NAME_WE)
        add_executable(${_name} ${_ex})
        target_link_libraries(${_name} Design)
    endforeach()
    add_executable(test_api_cpp test_api_cpp.cpp)
    target_link_libraries(test_api_cpp cost_model)
endif()

# Unit tests: cmake -DBUILD_COST_MODEL_TESTS=ON .. && make && ctest
option(BUILD_COST_MODEL_TESTS "Build cost model tests" OFF)
if(BUILD_COST_MODEL_TESTS)
    add_subdirectory(test)
endif()
