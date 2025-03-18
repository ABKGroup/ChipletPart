#!/bin/bash

# Define paths
BUILD_DIR="./build"
TEST_CASE_DIR="./test_cases/48_1_14_4_1600_1600"
EXECUTABLE="${BUILD_DIR}/bin/test_api_cpp"

# Check if the executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: test_api_cpp executable not found at ${EXECUTABLE}"
    echo "Make sure to build the project first by running ./build.sh"
    exit 1
fi

# Check if the test case directory exists
if [ ! -d "$TEST_CASE_DIR" ]; then
    echo "Error: Test case directory not found at ${TEST_CASE_DIR}"
    exit 1
fi

# Run test_api_cpp with the test case files
echo "Running test_api_cpp with test case from ${TEST_CASE_DIR}"
${EXECUTABLE} \
    "${TEST_CASE_DIR}/io_definitions.xml" \
    "${TEST_CASE_DIR}/layer_definitions.xml" \
    "${TEST_CASE_DIR}/assembly_process_definitions.xml" \
    "${TEST_CASE_DIR}/wafer_process_definitions.xml" \
    "${TEST_CASE_DIR}/test_definitions.xml" \
    "${TEST_CASE_DIR}/block_level_netlist_ws-48_1_14_4_1600_1600.xml" \
    "${TEST_CASE_DIR}/block_definitions_ws-48_1_14_4_1600_1600.txt"

# Check the exit code
exit_code=$?
if [ $exit_code -eq 0 ]; then
    echo "Test completed successfully"
else
    echo "Test failed with exit code $exit_code"
fi
exit $exit_code 
