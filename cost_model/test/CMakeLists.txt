cmake_minimum_required(VERSION 3.10)

# Find OpenMP
find_package(OpenMP)

# Tests for design components
add_executable(test_io_process test_io_process.cpp)
target_link_libraries(test_io_process Design)
if(OpenMP_CXX_FOUND)
    target_link_libraries(test_io_process ${OpenMP_CXX_LIBRARIES})
endif()

add_executable(test_wafer_process test_wafer_process.cpp)
target_link_libraries(test_wafer_process Design)
if(OpenMP_CXX_FOUND)
    target_link_libraries(test_wafer_process ${OpenMP_CXX_LIBRARIES})
endif()

add_executable(test_layer_process test_layer_process.cpp)
target_link_libraries(test_layer_process Design)
if(OpenMP_CXX_FOUND)
    target_link_libraries(test_layer_process ${OpenMP_CXX_LIBRARIES})
endif()

add_executable(test_test_process test_test_process.cpp)
target_link_libraries(test_test_process Design)
if(OpenMP_CXX_FOUND)
    target_link_libraries(test_test_process ${OpenMP_CXX_LIBRARIES})
endif()

add_executable(test_assembly_process test_assembly_process.cpp)
target_link_libraries(test_assembly_process Design)
if(OpenMP_CXX_FOUND)
    target_link_libraries(test_assembly_process ${OpenMP_CXX_LIBRARIES})
endif()

add_executable(test_chip_class test_chip_class.cpp)
target_link_libraries(test_chip_class Design)
if(OpenMP_CXX_FOUND)
    target_link_libraries(test_chip_class ${OpenMP_CXX_LIBRARIES})
endif()

# Add a custom target to run all tests
add_custom_target(run_tests
    COMMAND test_io_process
    COMMAND test_wafer_process
    COMMAND test_layer_process
    COMMAND test_test_process
    COMMAND test_assembly_process
    COMMAND test_chip_class
    COMMENT "Running all design test cases"
) 