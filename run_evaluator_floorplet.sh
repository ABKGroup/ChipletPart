#!/bin/bash

# Set colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to display help/usage information
show_help() {
    echo -e "${BLUE}Usage: $0 <test_case_name> <floorplet_file> [options]${NC}"
    echo
    echo "Options:"
    echo "  --reach <value>       Specify reach value (default: 0.50)"
    echo "  --separation <value>  Specify separation value (default: 0.25)"
    echo "  --tech <node>         Specify tech node for standard partitioning (default: 7nm)"
    echo "  --seed <value>        Specify random seed (default: 42)"
    echo "  --help                Display this help message"
    echo
    echo "Examples:"
    echo "  $0 48_1_14_4_1600_1600 floorplet/block_definitions.txt.floorplet_part.min_num_chiplets_10.max_num_chiplets_10 --tech 7nm"
    echo "  $0 48_1_14_4_1600_1600 floorplet/block_definitions.txt.floorplet_part.min_num_chiplets_8.max_num_chiplets_8 --reach 0.75"
}

# Check if not enough arguments were provided
if [ $# -lt 2 ]; then
    show_help
    exit 1
fi

# Parse the first argument as the test case name
TEST_CASE_NAME="$1"
FLOORPLET_FILE="$2"
shift 2  # Remove the first two arguments

# Default values
DEFAULT_REACH="0.50"
DEFAULT_SEPARATION="0.25"
DEFAULT_TECH="7nm"
DEFAULT_SEED="42"

# Parse command line arguments
while [ "$#" -gt 0 ]; do
    case "$1" in
        --help)
            show_help
            exit 0
            ;;
        --reach)
            DEFAULT_REACH="$2"
            shift 2
            ;;
        --separation)
            DEFAULT_SEPARATION="$2"
            shift 2
            ;;
        --tech)
            DEFAULT_TECH="$2"
            shift 2
            ;;
        --seed)
            DEFAULT_SEED="$2"
            shift 2
            ;;
        *)
            echo -e "${RED}Error: Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Define the base directory where the executable and test data are located
BASE_DIR="$(pwd)"
BUILD_DIR="${BASE_DIR}/build"
EXECUTABLE="${BUILD_DIR}/bin/chipletPart"
TEST_DATA_DIR="${BASE_DIR}/test_data/${TEST_CASE_NAME}"
FLOORPLET_PATH="${TEST_DATA_DIR}/${FLOORPLET_FILE}"

# Check if the floorplet file exists
if [ ! -f "$FLOORPLET_PATH" ]; then
    echo -e "${RED}Error: Floorplet file not found: $FLOORPLET_PATH${NC}"
    exit 1
fi

# Create results directory for this test case if it doesn't exist
mkdir -p "${BASE_DIR}/results/"

# Create a temporary partition file from the floorplet file
TEMP_PARTITION="${TEST_DATA_DIR}/manual.part"
echo -e "${BLUE}Converting floorplet file to partition format...${NC}"

# Use the conversion script to generate the partition file
./convert_floorplet_partition.sh "$FLOORPLET_PATH" "$TEMP_PARTITION"

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to convert floorplet file${NC}"
    exit 1
fi

# Define file paths for the input files
HYPERGRAPH_PART="$TEMP_PARTITION"
IO="${TEST_DATA_DIR}/io_definitions.xml"
LAYER="${TEST_DATA_DIR}/layer_definitions.xml"
WAFER="${TEST_DATA_DIR}/wafer_process_definitions.xml"
ASSEMBLY="${TEST_DATA_DIR}/assembly_process_definitions.xml"
TEST="${TEST_DATA_DIR}/test_definitions.xml"
NETLIST="${TEST_DATA_DIR}/block_level_netlist.xml"
BLOCKS="${TEST_DATA_DIR}/block_definitions.txt"

# Check that all required files exist
for file in "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}Error: File not found: $file${NC}"
        echo "Please ensure the test data exists in ${TEST_DATA_DIR}."
        exit 1
    fi
done

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Error: $EXECUTABLE not found${NC}"
    echo -e "${YELLOW}Make sure you have built the project with cmake${NC}"
    exit 1
fi

# Run the evaluator
echo -e "${GREEN}Running evaluator with the following parameters:${NC}"
echo -e "${BLUE}Partition file:        ${YELLOW}$HYPERGRAPH_PART${NC}"
echo -e "${BLUE}Original Floorplet:    ${YELLOW}$FLOORPLET_PATH${NC}"
echo -e "${BLUE}IO file:               ${YELLOW}$IO${NC}"
echo -e "${BLUE}Layer file:            ${YELLOW}$LAYER${NC}"
echo -e "${BLUE}Wafer process file:    ${YELLOW}$WAFER${NC}"
echo -e "${BLUE}Assembly process file: ${YELLOW}$ASSEMBLY${NC}"
echo -e "${BLUE}Test file:             ${YELLOW}$TEST${NC}"
echo -e "${BLUE}Netlist file:          ${YELLOW}$NETLIST${NC}"
echo -e "${BLUE}Blocks file:           ${YELLOW}$BLOCKS${NC}"
echo -e "${BLUE}Reach:                 ${YELLOW}$DEFAULT_REACH${NC}"
echo -e "${BLUE}Separation:            ${YELLOW}$DEFAULT_SEPARATION${NC}"
echo -e "${BLUE}Technology:            ${YELLOW}$DEFAULT_TECH${NC}"
echo -e ""

# Run the command for evaluation
echo -e "${GREEN}Executing evaluation...${NC}"
$EXECUTABLE \
    "$HYPERGRAPH_PART" \
    "$IO" \
    "$LAYER" \
    "$WAFER" \
    "$ASSEMBLY" \
    "$TEST" \
    "$NETLIST" \
    "$BLOCKS" \
    "$DEFAULT_REACH" \
    "$DEFAULT_SEPARATION" \
    "$DEFAULT_TECH"

# Check the exit status
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Evaluation completed successfully${NC}"
else
    echo -e "${RED}Evaluation failed with exit code $?${NC}"
fi 