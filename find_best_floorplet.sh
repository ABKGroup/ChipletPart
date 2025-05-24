#!/bin/bash

# Set colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Function to display help/usage information
show_help() {
    echo -e "${BLUE}Usage: $0 <test_case_name> [options]${NC}"
    echo
    echo "Options:"
    echo "  --reach <value>       Specify reach value (default: 0.50)"
    echo "  --separation <value>  Specify separation value (default: 0.25)"
    echo "  --tech <node>         Specify tech node for standard partitioning (default: 7nm)"
    echo "  --help                Display this help message"
    echo
    echo "Example:"
    echo "  $0 48_1_14_4_1600_1600 --separation 0.30"
    echo
    echo "This script will evaluate all floorplet partition files for the specified test case"
    echo "and return the one with the lowest cost."
}

# Function to compare two floating point numbers
# Returns 1 if first number is less than second number, 0 otherwise
float_lt() {
    result=$(awk -v a="$1" -v b="$2" 'BEGIN { print (a < b) ? "true" : "false" }')
    [ "$result" == "true" ]
}

# Check if no arguments were provided
if [ $# -lt 1 ]; then
    show_help
    exit 1
fi

# Parse the first argument as the test case name
TEST_CASE_NAME="$1"
shift  # Remove the first argument

# Default values
REACH="0.50"
SEPARATION="0.25"
TECH="7nm"
EXTRA_ARGS=""

# Parse command line arguments
while [ "$#" -gt 0 ]; do
    case "$1" in
        --help)
            show_help
            exit 0
            ;;
        --reach)
            REACH="$2"
            shift 2
            ;;
        --separation)
            SEPARATION="$2"
            shift 2
            ;;
        --tech)
            TECH="$2"
            shift 2
            ;;
        *)
            EXTRA_ARGS="$EXTRA_ARGS $1"
            shift
            ;;
    esac
done

# Define paths
BASE_DIR="$(pwd)"
TEST_DATA_DIR="${BASE_DIR}/test_data/${TEST_CASE_NAME}"
FLOORPLET_DIR="${TEST_DATA_DIR}/floorplet"

# Check if the test case directory exists
if [ ! -d "$TEST_DATA_DIR" ]; then
    echo -e "${RED}Error: Test case directory not found: $TEST_DATA_DIR${NC}"
    exit 1
fi

# Check if the floorplet directory exists
if [ ! -d "$FLOORPLET_DIR" ]; then
    echo -e "${RED}Error: Floorplet directory not found: $FLOORPLET_DIR${NC}"
    exit 1
fi

# Find all floorplet partition files
FLOORPLET_FILES=$(find "$FLOORPLET_DIR" -name "*.floorplet_part.*" | sort)

# Check if any floorplet files were found
if [ -z "$FLOORPLET_FILES" ]; then
    echo -e "${RED}Error: No floorplet partition files found in $FLOORPLET_DIR${NC}"
    exit 1
fi

echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo -e "${CYAN}${BOLD}    Finding Best Floorplet Partition for $TEST_CASE_NAME${NC}"
echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo -e "${BLUE}Test case directory: ${YELLOW}$TEST_DATA_DIR${NC}"
echo -e "${BLUE}Floorplet directory: ${YELLOW}$FLOORPLET_DIR${NC}"
echo -e "${BLUE}Number of floorplet files: ${YELLOW}$(echo "$FLOORPLET_FILES" | wc -l)${NC}"
echo -e "${BLUE}Parameters: reach=${YELLOW}$REACH${BLUE}, separation=${YELLOW}$SEPARATION${BLUE}, tech=${YELLOW}$TECH${NC}"
echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo

# Initialize variables to track the best result
BEST_COST=999999999
BEST_FILE=""
BEST_PARTITIONS=0
BEST_FEASIBILITY=0
BEST_OUTPUT=""

# Create a results table header
printf "${BOLD}| %-60s | %-10s | %-10s | %-10s |${NC}\n" "Floorplet File" "Cost" "Partitions" "Feasible"
echo "-----------------------------------------------------------------------------------------"

# Process each floorplet file
for FLOORPLET_FILE in $FLOORPLET_FILES; do
    # Get the relative path for display
    REL_PATH="${FLOORPLET_FILE#$TEST_DATA_DIR/}"
    
    echo -e "${YELLOW}Evaluating: $REL_PATH${NC}"
    
    # Run the evaluator and capture output
    OUTPUT=$(./run_evaluator_floorplet.sh "$TEST_CASE_NAME" "$REL_PATH" --reach "$REACH" --separation "$SEPARATION" --tech "$TECH" $EXTRA_ARGS 2>&1)
    
    # Extract cost, partitions, and feasibility using more precise grep patterns
    COST=$(echo "$OUTPUT" | grep -o "Cost of partition is [0-9.]*" | awk '{print $NF}')
    PARTITIONS=$(echo "$OUTPUT" | grep -o "Number of partitions is [0-9]*" | awk '{print $NF}')
    FEASIBILITY=$(echo "$OUTPUT" | grep -o "Floorplan feasibility is [0-1]" | awk '{print $NF}')
    
    # Handle case where the evaluation might have failed
    if [ -z "$COST" ]; then
        echo -e "  ${RED}Failed to extract cost - evaluation may have failed${NC}"
        printf "| ${RED}%-60s${NC} | ${RED}%-10s${NC} | ${RED}%-10s${NC} | ${RED}%-10s${NC} |\n" "$REL_PATH" "FAILED" "N/A" "N/A"
        continue
    fi
    
    # Display result for this file
    if [ "$FEASIBILITY" -eq 1 ]; then
        printf "| %-60s | ${GREEN}%-10s${NC} | %-10s | ${GREEN}%-10s${NC} |\n" "$REL_PATH" "$COST" "$PARTITIONS" "YES"
    else
        printf "| %-60s | ${YELLOW}%-10s${NC} | %-10s | ${RED}%-10s${NC} |\n" "$REL_PATH" "$COST" "$PARTITIONS" "NO"
    fi
    
    # Update the best result if this one is better
    # We prioritize feasible solutions, then lower cost
    if [ "$FEASIBILITY" -eq 1 ] && ([ "$BEST_FEASIBILITY" -eq 0 ] || float_lt "$COST" "$BEST_COST"); then
        BEST_COST=$COST
        BEST_FILE=$REL_PATH
        BEST_PARTITIONS=$PARTITIONS
        BEST_FEASIBILITY=$FEASIBILITY
        BEST_OUTPUT=$OUTPUT
    elif [ "$FEASIBILITY" -eq 0 ] && [ "$BEST_FEASIBILITY" -eq 0 ] && ([ -z "$BEST_FILE" ] || float_lt "$COST" "$BEST_COST"); then
        BEST_COST=$COST
        BEST_FILE=$REL_PATH
        BEST_PARTITIONS=$PARTITIONS
        BEST_FEASIBILITY=$FEASIBILITY
        BEST_OUTPUT=$OUTPUT
    fi
done

echo "-----------------------------------------------------------------------------------------"
echo

# Display the best result
echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo -e "${CYAN}${BOLD}                     Best Result                         ${NC}"
echo -e "${CYAN}${BOLD}=========================================================${NC}"

if [ -z "$BEST_FILE" ]; then
    echo -e "${RED}No valid results found.${NC}"
else
    echo -e "${BLUE}Best floorplet file: ${GREEN}$BEST_FILE${NC}"
    echo -e "${BLUE}Cost: ${GREEN}$BEST_COST${NC}"
    echo -e "${BLUE}Number of partitions: ${GREEN}$BEST_PARTITIONS${NC}"
    if [ "$BEST_FEASIBILITY" -eq 1 ]; then
        echo -e "${BLUE}Floorplan feasibility: ${GREEN}YES${NC}"
    else
        echo -e "${BLUE}Floorplan feasibility: ${RED}NO${NC}"
    fi
    
    # Save the best result
    echo "$BEST_OUTPUT" > "${BASE_DIR}/results/${TEST_CASE_NAME}_best_floorplet_result.txt"
    echo -e "${BLUE}Detailed output saved to: ${GREEN}${BASE_DIR}/results/${TEST_CASE_NAME}_best_floorplet_result.txt${NC}"
fi

echo -e "${CYAN}${BOLD}=========================================================${NC}" 