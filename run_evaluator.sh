#!/bin/bash

# Set colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to display help/usage information
show_help() {
    echo -e "${BLUE}Usage: $0 <test_case_name> [options]${NC}"
    echo
    echo "Options:"
    echo "  --reach <value>       Specify reach value (default: 0.50)"
    echo "  --separation <value>  Specify separation value (default: 0.25)"
    echo "  --tech <node>         Specify tech node for standard partitioning (default: 7nm)"
    echo "  --seed <value>        Specify random seed (default: 42)"
    echo "  --genetic             Use genetic tech partitioning algorithm"
    echo "  --tech-nodes <nodes>  Specify comma-separated list of tech nodes for genetic partitioning"
    echo "                        Example: --tech-nodes 7nm,10nm,14nm,28nm"
    echo "  --generations <num>   Specify number of generations for genetic algorithm (default: 50)"
    echo "  --population <num>    Specify population size for genetic algorithm (default: 50)"
    echo "  --help                Display this help message"
    echo
    echo "Examples:"
    echo "  $0 design1 --tech 5nm"
    echo "  $0 design2 --genetic --tech-nodes 7nm,10nm,14nm --seed 123"
}

# Check if no arguments were provided
if [ $# -eq 0 ]; then
    show_help
    exit 1
fi

# Parse the first argument as the test case name
TEST_CASE_NAME="$1"
shift  # Remove the first argument

# Default values
DEFAULT_REACH="0.50"
DEFAULT_SEPARATION="0.25"
DEFAULT_TECH="7nm"
DEFAULT_SEED="42"
DEFAULT_GENERATIONS="50"
DEFAULT_POPULATION="50"
USE_GENETIC=false
TECH_NODES=""

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
        --genetic)
            USE_GENETIC=true
            shift
            ;;
        --tech-nodes)
            TECH_NODES="$2"
            shift 2
            ;;
        --generations)
            DEFAULT_GENERATIONS="$2"
            shift 2
            ;;
        --population)
            DEFAULT_POPULATION="$2"
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

# Create results directory for this test case if it doesn't exist
mkdir -p "${BASE_DIR}/results/"

# Define file paths for the input files
HYPERGRAPH_PART="${TEST_DATA_DIR}/manual.part"
IO="${TEST_DATA_DIR}/io_definitions.xml"
LAYER="${TEST_DATA_DIR}/layer_definitions.xml"
WAFER="${TEST_DATA_DIR}/wafer_process_definitions.xml"
ASSEMBLY="${TEST_DATA_DIR}/assembly_process_definitions.xml"
TEST="${TEST_DATA_DIR}/test_definitions.xml"
NETLIST="${TEST_DATA_DIR}/block_level_netlist.xml"
BLOCKS="${TEST_DATA_DIR}/block_definitions.txt"

# Check that all required files exist
for file in "$HYPERGRAPH_PART" "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS"; do
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

# Run the command
echo -e "${GREEN}Executing evaluation...${NC}"

if [ "$USE_GENETIC" = true ]; then
    # Convert comma-separated tech nodes to proper format
    TECH_NODES_ARRAY=(${TECH_NODES//,/ })
    TECH_ARGS=""
    for node in "${TECH_NODES_ARRAY[@]}"; do
        TECH_ARGS="$TECH_ARGS --tech-nodes $node"
    done
    
    # Execute with genetic algorithm parameters
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
        "--genetic-tech-part" \
        $TECH_ARGS \
        "--generations" "$DEFAULT_GENERATIONS" \
        "--population" "$DEFAULT_POPULATION" \
        "--seed" "$DEFAULT_SEED"
else
    # Standard evaluation
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
fi

# Check the exit status
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Evaluation completed successfully${NC}"
else
    echo -e "${RED}Evaluation failed with exit code $?${NC}"
fi 