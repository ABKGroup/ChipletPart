#!/bin/bash

# Set colors for better output (optional)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Define base directories and files
BASE_DIR="$(pwd)"
BUILD_DIR="${BASE_DIR}/build"
EXECUTABLE="${BUILD_DIR}/bin/chipletPart"
TEST_DATA_PARENT="${BASE_DIR}/test_data"
RESULTS_DIR="${BASE_DIR}/results"
JOBS_FILE="${BASE_DIR}/jobs.txt"

# Default parameters
DEFAULT_REACH="0.50"
DEFAULT_SEPARATION="0.25"
DEFAULT_TECH="7nm"
DEFAULT_SEED="42"

# Create results directory and clear jobs file
mkdir -p "${RESULTS_DIR}"
> "${JOBS_FILE}"

# Loop over each test case directory in the test_data parent folder
for test_case in "${TEST_DATA_PARENT}"/*/ ; do
    # Remove any trailing slash
    test_case="${test_case%/}"
    TESTCASE_NAME=$(basename "$test_case")

    # Construct file paths
    IO="${test_case}/io_definitions.xml"
    LAYER="${test_case}/layer_definitions.xml"
    WAFER="${test_case}/wafer_process_definitions.xml"
    ASSEMBLY="${test_case}/assembly_process_definitions.xml"
    TEST="${test_case}/test_definitions.xml"
    NETLIST="${test_case}/block_level_netlist.xml"
    BLOCKS="${test_case}/block_definitions.txt"

    # Check that all required files exist; skip if any file is missing
    for file in "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS"; do
        if [ ! -f "$file" ]; then
            echo -e "${RED}Warning: File not found: $file. Skipping test case ${TESTCASE_NAME}.${NC}"
            continue 2
        fi
    done

    # Derive the base name of the netlist file and append the test case folder name
    NETLIST_BASE=$(basename "$NETLIST" .xml)
    LOG_FILE="${RESULTS_DIR}/${NETLIST_BASE}_${TESTCASE_NAME}.log"

    # Build the command string. The command runs the executable with its arguments and tees the output to the log file.
    CMD="\"${EXECUTABLE}\" \"${IO}\" \"${LAYER}\" \"${WAFER}\" \"${ASSEMBLY}\" \"${TEST}\" \"${NETLIST}\" \"${BLOCKS}\" \"${DEFAULT_REACH}\" \"${DEFAULT_SEPARATION}\" \"${DEFAULT_TECH}\" --seed \"${DEFAULT_SEED}\" 2>&1 | tee -a \"${LOG_FILE}\""
    
    # Append the command to the jobs file
    echo "$CMD" >> "${JOBS_FILE}"
done

echo -e "${GREEN}Jobs file created at ${JOBS_FILE}.${NC}"
