# Design-class unit tests. Each test is a standalone executable that returns
# non-zero on failure; they are registered with CTest.
set(COST_MODEL_TESTS
    test_io_process
    test_wafer_process
    test_layer_process
    test_test_process
    test_assembly_process
    test_chip_class)

foreach(_t ${COST_MODEL_TESTS})
    add_executable(${_t} ${_t}.cpp)
    target_link_libraries(${_t} Design)
    add_test(NAME ${_t} COMMAND ${_t} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
