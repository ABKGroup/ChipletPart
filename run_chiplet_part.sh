#!/bin/bash

# Set colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Define the base directory where the executable and test data are located
BASE_DIR="$(pwd)"
BUILD_DIR="${BASE_DIR}/build"
EXECUTABLE="${BUILD_DIR}/bin/chipletPart"
TEST_DATA_PARENT="${BASE_DIR}/test_data"

# Default parameters
DEFAULT_REACH="0.50"
DEFAULT_SEPARATION="0.25"
DEFAULT_TECH="7nm"
DEFAULT_SEED="42"

# Create directory for results if it doesn't exist
RESULTS_DIR="${BASE_DIR}/results"
mkdir -p "${RESULTS_DIR}"

# Function to run a single test case
run_test() {
    local test_dir="$1"
    local reach="$2"
    local separation="$3"
    local tech="$4"
    local seed="$5"

    local TESTCASE_NAME
    TESTCASE_NAME=$(basename "$test_dir")

    echo -e "${YELLOW}==============================================${NC}"
    echo -e "${YELLOW}Running test case: ${TESTCASE_NAME}${NC}"
    echo -e "${YELLOW}==============================================${NC}"

    # Construct file paths
    local IO="${test_dir}/io_definitions.xml"
    local LAYER="${test_dir}/layer_definitions.xml"
    local WAFER="${test_dir}/wafer_process_definitions.xml"
    local ASSEMBLY="${test_dir}/assembly_process_definitions.xml"
    local TEST="${test_dir}/test_definitions.xml"
    local NETLIST="${test_dir}/block_level_netlist.xml"
    local BLOCKS="${test_dir}/block_definitions.txt"

    # Check if required files exist
    for file in "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS"; do
        if [ ! -f "$file" ]; then
            echo -e "${RED}Error: File not found: $file${NC}"
            echo "Skipping test case ${TESTCASE_NAME}"
            return 1
        fi
    done

    # Check if executable exists
    if [ ! -f "$EXECUTABLE" ]; then
        echo -e "${RED}Error: Executable not found at $EXECUTABLE${NC}"
        echo "Please build the project first with:"
        echo "  mkdir -p build && cd build && cmake .. && make"
        exit 1
    fi

    # Derive base name of the netlist file (without .xml) and append the test case folder name
    NETLIST_BASE=$(basename "$NETLIST" .xml)
    LOG_FILE="${RESULTS_DIR}/${NETLIST_BASE}_${TESTCASE_NAME}.log"

    echo -e "${GREEN}Executing chipletPart for test case ${TESTCASE_NAME}...${NC}"
    echo -e "${GREEN}Logging output to ${LOG_FILE}${NC}"

    # Run the chipletPart executable and tee output to the log file
    "$EXECUTABLE" \
        "$IO" \
        "$LAYER" \
        "$WAFER" \
        "$ASSEMBLY" \
        "$TEST" \
        "$NETLIST" \
        "$BLOCKS" \
        "$reach" \
        "$separation" \
        "$tech" \
        --seed "$seed" 2>&1 | tee -a "$LOG_FILE"

    local exit_code=${PIPESTATUS[0]}
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}Test case ${TESTCASE_NAME} completed successfully!${NC}"
    else
        echo -e "${RED}Test case ${TESTCASE_NAME} failed with exit code $exit_code${NC}"
    fi
}

# Loop over each test case directory in the test_data parent folder
for test_case in "${TEST_DATA_PARENT}"/*/ ; do
    # Remove any trailing slash for cleaner printing
    test_case="${test_case%/}"
    run_test "$test_case" "$DEFAULT_REACH" "$DEFAULT_SEPARATION" "$DEFAULT_TECH" "$DEFAULT_SEED"
done

echo -e "\n${GREEN}All tests completed. Logs are available in ${RESULTS_DIR}.${NC}"
