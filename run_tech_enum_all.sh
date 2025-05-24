#!/bin/bash

# Script to run technology enumeration on all test designs
# Author: AI Assistant
# Date: $(date +%F)

# Colors for terminal output
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[1;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT="./run_chiplet_test.sh"
LOG_DIR="logs"
TECH_NODES="7nm,10nm,14nm"  # Default technology nodes
MAX_PARTITIONS=3            # Default max partitions
TIMEOUT=14400               # 4 hour timeout per design (in seconds)

# Create logs directory if it doesn't exist
mkdir -p "$LOG_DIR"

# Function to display script usage
show_usage() {
    echo -e "${BLUE}Usage:${NC} $0 [options]"
    echo
    echo "Options:"
    echo "  --tech-nodes <list>  Specify comma-separated list of technology nodes (default: $TECH_NODES)"
    echo "  --max-parts <num>    Maximum number of partitions to consider (default: $MAX_PARTITIONS)"
    echo "  --timeout <seconds>  Timeout for each test case in seconds (default: $TIMEOUT)"
    echo "  --help               Show this help message"
    echo
    echo "This script will run technology enumeration on all test designs in test_data/"
    echo "and log the results to individual log files in $LOG_DIR/"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --tech-nodes)
            TECH_NODES="$2"
            shift 2
            ;;
        --max-parts)
            MAX_PARTITIONS="$2"
            shift 2
            ;;
        --timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option:${NC} $1"
            show_usage
            exit 1
            ;;
    esac
done

# Check if run_chiplet_test.sh exists and is executable
if [[ ! -x "$SCRIPT" ]]; then
    echo -e "${RED}Error:${NC} $SCRIPT does not exist or is not executable"
    exit 1
fi

# Check if test_data directory exists
if [[ ! -d "test_data" ]]; then
    echo -e "${RED}Error:${NC} test_data directory does not exist"
    exit 1
fi

# Get all design directories
DESIGNS=($(ls -d test_data/* | xargs -n1 basename))

# Display summary before starting
echo -e "${BLUE}=== Technology Enumeration Test Suite ===${NC}"
echo -e "${BLUE}Found ${#DESIGNS[@]} test designs:${NC}"
for design in "${DESIGNS[@]}"; do
    echo "  - $design"
done
echo
echo -e "${BLUE}Configuration:${NC}"
echo "  Technology nodes: $TECH_NODES"
echo "  Max partitions: $MAX_PARTITIONS"
echo "  Timeout: $TIMEOUT seconds"
echo "  Log directory: $LOG_DIR"
echo 
echo -e "${BLUE}=== Starting Test Run ===${NC}"

# Record start time
START_TIME=$(date +%s)

# Counter for successful/failed runs
SUCCESS_COUNT=0
FAIL_COUNT=0

# Run each design
for design in "${DESIGNS[@]}"; do
    echo -e "\n${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} Processing design: ${YELLOW}$design${NC}"
    LOG_FILE="$LOG_DIR/${design}.log"
    
    # Log header information
    echo "==== Technology Enumeration Log for $design ====" > "$LOG_FILE"
    echo "Date: $(date '+%Y-%m-%d %H:%M:%S')" >> "$LOG_FILE"
    echo "Technology Nodes: $TECH_NODES" >> "$LOG_FILE"
    echo "Max Partitions: $MAX_PARTITIONS" >> "$LOG_FILE"
    echo "Command: $SCRIPT $design --tech-enum --tech-nodes $TECH_NODES --max-partitions $MAX_PARTITIONS" >> "$LOG_FILE"
    echo "=====================================================" >> "$LOG_FILE"
    echo >> "$LOG_FILE"
    
    # Run the command with timeout and tee output to both terminal and log file
    echo -e "${GREEN}Running:${NC} $SCRIPT $design --tech-enum --tech-nodes $TECH_NODES --max-partitions $MAX_PARTITIONS"
    echo -e "${GREEN}Logging to:${NC} $LOG_FILE"
    
    # Start timer for this specific test case
    TEST_START_TIME=$(date +%s)
    
    # Run the command with timeout
    if timeout $TIMEOUT $SCRIPT "$design" --tech-enum --tech-nodes "$TECH_NODES" --max-partitions "$MAX_PARTITIONS" 2>&1 | tee -a "$LOG_FILE"; then
        STATUS="Success"
        ((SUCCESS_COUNT++))
    else
        EXIT_CODE=$?
        STATUS="Failed"
        ((FAIL_COUNT++))
        if [[ $EXIT_CODE -eq 124 ]]; then
            echo -e "\n${RED}[TIMEOUT]${NC} Test case $design timed out after $TIMEOUT seconds" | tee -a "$LOG_FILE"
            STATUS="Timeout"
        else
            echo -e "\n${RED}[FAILED]${NC} Test case $design failed with exit code $EXIT_CODE" | tee -a "$LOG_FILE"
        fi
    fi
    
    # Calculate and log the elapsed time for this test case
    TEST_END_TIME=$(date +%s)
    TEST_ELAPSED=$((TEST_END_TIME - TEST_START_TIME))
    
    # Log the completion status and time
    echo -e "\n=====================================================" >> "$LOG_FILE"
    echo "Test completed with status: $STATUS" >> "$LOG_FILE"
    echo "Elapsed time: $TEST_ELAPSED seconds" >> "$LOG_FILE"
    echo "=====================================================" >> "$LOG_FILE"
    
    echo -e "${GREEN}Completed:${NC} $design ($STATUS) in $TEST_ELAPSED seconds"
    echo -e "${BLUE}Progress:${NC} $((SUCCESS_COUNT + FAIL_COUNT))/${#DESIGNS[@]} designs processed"
done

# Calculate and display total elapsed time
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
HOURS=$((ELAPSED / 3600))
MINUTES=$(((ELAPSED % 3600) / 60))
SECONDS=$((ELAPSED % 60))

echo -e "\n${BLUE}=== Test Run Complete ===${NC}"
echo -e "${GREEN}Successfully completed:${NC} $SUCCESS_COUNT/${#DESIGNS[@]} designs"
echo -e "${RED}Failed:${NC} $FAIL_COUNT/${#DESIGNS[@]} designs"
echo -e "${BLUE}Total time:${NC} ${HOURS}h ${MINUTES}m ${SECONDS}s"
echo -e "${BLUE}Log files are available in:${NC} $LOG_DIR/"

# Generate a summary file
SUMMARY_FILE="$LOG_DIR/summary.txt"
echo "Technology Enumeration Test Suite Summary" > "$SUMMARY_FILE"
echo "Date: $(date '+%Y-%m-%d %H:%M:%S')" >> "$SUMMARY_FILE"
echo "Technology Nodes: $TECH_NODES" >> "$SUMMARY_FILE"
echo "Max Partitions: $MAX_PARTITIONS" >> "$SUMMARY_FILE"
echo "Total Designs: ${#DESIGNS[@]}" >> "$SUMMARY_FILE"
echo "Successful: $SUCCESS_COUNT" >> "$SUMMARY_FILE"
echo "Failed: $FAIL_COUNT" >> "$SUMMARY_FILE"
echo "Total Runtime: ${HOURS}h ${MINUTES}m ${SECONDS}s" >> "$SUMMARY_FILE"
echo >> "$SUMMARY_FILE"
echo "Individual Test Results:" >> "$SUMMARY_FILE"

# Extract and add individual test result summaries
for design in "${DESIGNS[@]}"; do
    LOG_FILE="$LOG_DIR/${design}.log"
    
    # Extract status and runtime from log file
    if grep -q "Test completed with status: Success" "$LOG_FILE"; then
        STATUS="Success"
    elif grep -q "Test completed with status: Timeout" "$LOG_FILE"; then
        STATUS="Timeout"
    else
        STATUS="Failed"
    fi
    
    # Extract elapsed time
    ELAPSED_TIME=$(grep "Elapsed time:" "$LOG_FILE" | awk '{print $3}')
    
    # Extract canonical assignments if successful
    if [[ "$STATUS" == "Success" ]]; then
        CANONICAL_FORMS=$(grep "Unique canonical assignments:" "$LOG_FILE" | tail -1 | awk '{print $NF}')
        BEST_COST=$(grep "Best Cost:" "$LOG_FILE" | tail -1 | awk '{print $NF}')
        echo "$design: $STATUS, Time: ${ELAPSED_TIME}s, Canonical Forms: ${CANONICAL_FORMS:-N/A}, Best Cost: ${BEST_COST:-N/A}" >> "$SUMMARY_FILE"
    else
        echo "$design: $STATUS, Time: ${ELAPSED_TIME:-N/A}s" >> "$SUMMARY_FILE"
    fi
done

echo -e "${GREEN}Summary written to:${NC} $SUMMARY_FILE"
exit 0 