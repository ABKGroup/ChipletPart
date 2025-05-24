#!/bin/bash

# Set colors for better output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for test case folder argument
if [ -z "$1" ]; then
    echo -e "${RED}Error: No test case folder provided.${NC}"
    echo "Usage: $0 <test_case_folder>"
    exit 1
fi

# Get the test case folder name from argument
TEST_CASE_FOLDER="$1"

# Define the base directories
BASE_DIR="$(pwd)"
# Now TEST_DATA_DIR adapts to the test case name, e.g. ./test_data/48_1_14_4_1600_1600
TEST_DATA_DIR="${BASE_DIR}/test_data/${TEST_CASE_FOLDER}"
# Use the given parent directory for cost model test cases
COST_MODEL_PARENT="/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/cost_model/test_cases"
COST_MODEL_TEST_CASE_DIR="${COST_MODEL_PARENT}/${TEST_CASE_FOLDER}"

echo -e "${YELLOW}Setting up test data in ${TEST_DATA_DIR}...${NC}"

# Create test_data directory if it doesn't exist
if [ ! -d "$TEST_DATA_DIR" ]; then
    mkdir -p "$TEST_DATA_DIR"
    echo "Created directory: $TEST_DATA_DIR"
fi

# Check if cost model test case directory exists
if [ ! -d "$COST_MODEL_TEST_CASE_DIR" ]; then
    echo -e "${RED}Error: Cost model test case directory not found at ${COST_MODEL_TEST_CASE_DIR}${NC}"
    echo "Please make sure the ${TEST_CASE_FOLDER} directory exists under ${COST_MODEL_PARENT}."
    exit 1
fi

# Copy files from cost model test case directory to test_data directory
echo "Copying files from cost model test case directory..."

# Define source files with dynamic test case directory
SOURCE_IO="${COST_MODEL_TEST_CASE_DIR}/io_definitions.xml"
SOURCE_LAYER="${COST_MODEL_TEST_CASE_DIR}/layer_definitions.xml"
SOURCE_WAFER="${COST_MODEL_TEST_CASE_DIR}/wafer_process_definitions.xml"
SOURCE_ASSEMBLY="${COST_MODEL_TEST_CASE_DIR}/assembly_process_definitions.xml"
SOURCE_TEST="${COST_MODEL_TEST_CASE_DIR}/test_definitions.xml"
SOURCE_NETLIST="${COST_MODEL_TEST_CASE_DIR}/block_level_netlist_ws-${TEST_CASE_FOLDER}.xml"
SOURCE_BLOCKS="${COST_MODEL_TEST_CASE_DIR}/block_definitions_ws-${TEST_CASE_FOLDER}.txt"

# Define destination files with dynamic test_data directory
DEST_IO="${TEST_DATA_DIR}/io_definitions.xml"
DEST_LAYER="${TEST_DATA_DIR}/layer_definitions.xml"
DEST_WAFER="${TEST_DATA_DIR}/wafer_process_definitions.xml"
DEST_ASSEMBLY="${TEST_DATA_DIR}/assembly_process_definitions.xml"
DEST_TEST="${TEST_DATA_DIR}/test_definitions.xml"
DEST_NETLIST="${TEST_DATA_DIR}/block_level_netlist.xml"
DEST_BLOCKS="${TEST_DATA_DIR}/block_definitions.txt"

# Copy each file and check if copy succeeded
success=true

# IO definitions
if [ -f "$SOURCE_IO" ]; then
    cp "$SOURCE_IO" "$DEST_IO"
    echo "  - Copied: $(basename "$SOURCE_IO") -> $(basename "$DEST_IO")"
else
    echo -e "${RED}  - Error: Source file not found: $SOURCE_IO${NC}"
    success=false
fi

# Layer definitions
if [ -f "$SOURCE_LAYER" ]; then
    cp "$SOURCE_LAYER" "$DEST_LAYER"
    echo "  - Copied: $(basename "$SOURCE_LAYER") -> $(basename "$DEST_LAYER")"
else
    echo -e "${RED}  - Error: Source file not found: $SOURCE_LAYER${NC}"
    success=false
fi

# Wafer process definitions
if [ -f "$SOURCE_WAFER" ]; then
    cp "$SOURCE_WAFER" "$DEST_WAFER"
    echo "  - Copied: $(basename "$SOURCE_WAFER") -> $(basename "$DEST_WAFER")"
else
    echo -e "${RED}  - Error: Source file not found: $SOURCE_WAFER${NC}"
    success=false
fi

# Assembly process definitions
if [ -f "$SOURCE_ASSEMBLY" ]; then
    cp "$SOURCE_ASSEMBLY" "$DEST_ASSEMBLY"
    echo "  - Copied: $(basename "$SOURCE_ASSEMBLY") -> $(basename "$DEST_ASSEMBLY")"
else
    echo -e "${RED}  - Error: Source file not found: $SOURCE_ASSEMBLY${NC}"
    success=false
fi

# Test definitions
if [ -f "$SOURCE_TEST" ]; then
    cp "$SOURCE_TEST" "$DEST_TEST"
    echo "  - Copied: $(basename "$SOURCE_TEST") -> $(basename "$DEST_TEST")"
else
    echo -e "${RED}  - Error: Source file not found: $SOURCE_TEST${NC}"
    success=false
fi

# Netlist
if [ -f "$SOURCE_NETLIST" ]; then
    cp "$SOURCE_NETLIST" "$DEST_NETLIST"
    echo "  - Copied: $(basename "$SOURCE_NETLIST") -> $(basename "$DEST_NETLIST")"
else
    echo -e "${RED}  - Error: Source file not found: $SOURCE_NETLIST${NC}"
    success=false
fi

# Block definitions
if [ -f "$SOURCE_BLOCKS" ]; then
    cp "$SOURCE_BLOCKS" "$DEST_BLOCKS"
    echo "  - Copied: $(basename "$SOURCE_BLOCKS") -> $(basename "$DEST_BLOCKS")"
else
    echo -e "${RED}  - Error: Source file not found: $SOURCE_BLOCKS${NC}"
    success=false
fi

# Check if all files were copied successfully
if [ "$success" = true ]; then
    echo -e "${GREEN}All files were copied successfully!${NC}"
    echo "You can now run the test script using:"
    echo "  ./run_chiplet_test.sh"
else
    echo -e "${RED}Some files could not be copied. Please check the errors above.${NC}"
    exit 1
fi 
