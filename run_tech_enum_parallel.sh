#!/bin/bash

# Script to run technology enumeration jobs in parallel
# Author: AI Assistant
# Date: $(date +%F)

# Colors for terminal output
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[1;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
SCRIPT="./run_chiplet_test.sh"
LOG_DIR="logs"
TECH_NODES="7nm,10nm,14nm"  # Default technology nodes
MAX_PARTITIONS=3            # Default max partitions
MAX_PARALLEL_JOBS=2         # Default maximum number of parallel jobs

# Create logs directory if it doesn't exist
mkdir -p "$LOG_DIR"

# Function to display script usage
show_usage() {
    echo -e "${BLUE}Usage:${NC} $0 [options]"
    echo
    echo "Options:"
    echo "  --tech-nodes <list>    Specify comma-separated list of technology nodes (default: $TECH_NODES)"
    echo "  --max-parts <num>      Maximum number of partitions to consider (default: $MAX_PARTITIONS)"
    echo "  --parallel <num>       Maximum number of parallel jobs (default: $MAX_PARALLEL_JOBS)"
    echo "  --help                 Show this help message"
    echo
    echo "This script will run technology enumeration on all test designs in test_data/"
    echo "with multiple jobs running in parallel, and log the results to individual log files in $LOG_DIR/"
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
        --parallel)
            MAX_PARALLEL_JOBS="$2"
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

# Get all design directories - list only actual design directories
DESIGNS=()
for dir in test_data/*; do
    # Skip if not a directory or if it's a system file
    if [[ ! -d "$dir" ]]; then
        continue
    fi
    
    # Get the basename
    design=$(basename "$dir")
    
    # Skip XML and other non-design files
    if [[ "$design" == "README.md" || 
          "$design" == *.txt || 
          "$design" == *.out ||
          "$design" == test_*.xml ]]; then
        continue
    fi
    
    # Add to valid designs
    DESIGNS+=("$design")
done

# Display summary before starting
echo -e "${BLUE}=== Parallel Technology Enumeration Test Suite ===${NC}"
echo -e "${BLUE}Found ${#DESIGNS[@]} test designs:${NC}"
for design in "${DESIGNS[@]}"; do
    echo "  - $design"
done

echo
echo -e "${BLUE}Configuration:${NC}"
echo "  Technology nodes: $TECH_NODES"
echo "  Max partitions: $MAX_PARTITIONS"
echo "  Max parallel jobs: $MAX_PARALLEL_JOBS"
echo "  Log directory: $LOG_DIR"
echo 
echo -e "${BLUE}=== Starting Parallel Test Run ===${NC}"

# Record start time
START_TIME=$(date +%s)

# Prepare a status file to track running and completed jobs
STATUS_FILE="$LOG_DIR/job_status.txt"
echo "Job Status File - $(date '+%Y-%m-%d %H:%M:%S')" > "$STATUS_FILE"
echo "Total jobs: ${#DESIGNS[@]}" >> "$STATUS_FILE"
echo "-------------------------------------------" >> "$STATUS_FILE"

# Function to run a single test case
run_test_case() {
    local design=$1
    local log_file="$LOG_DIR/${design}.log"
    local status_file=$2
    
    # Log header information
    echo "==== Technology Enumeration Log for $design ====" > "$log_file"
    echo "Date: $(date '+%Y-%m-%d %H:%M:%S')" >> "$log_file"
    echo "Technology Nodes: $TECH_NODES" >> "$log_file"
    echo "Max Partitions: $MAX_PARTITIONS" >> "$log_file"
    echo "Command: $SCRIPT $design --tech-enum --tech-nodes $TECH_NODES --max-partitions $MAX_PARTITIONS" >> "$log_file"
    echo "=====================================================" >> "$log_file"
    echo >> "$log_file"
    
    # Update status file to indicate job is running
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] STARTED: $design" >> "$status_file"
    
    # Start timer for this specific test case
    local test_start_time=$(date +%s)
    
    # Run the command
    if $SCRIPT "$design" --tech-enum --tech-nodes "$TECH_NODES" --max-partitions "$MAX_PARTITIONS" >> "$log_file" 2>&1; then
        local status="Success"
    else
        local status="Failed"
        echo -e "\n[FAILED] Test case $design failed with exit code $?" >> "$log_file"
    fi
    
    # Calculate and log the elapsed time for this test case
    local test_end_time=$(date +%s)
    local test_elapsed=$((test_end_time - test_start_time))
    
    # Log the completion status and time
    echo -e "\n=====================================================" >> "$log_file"
    echo "Test completed with status: $status" >> "$log_file"
    echo "Elapsed time: $test_elapsed seconds" >> "$log_file"
    echo "=====================================================" >> "$log_file"
    
    # Update status file to indicate job is complete
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] COMPLETED: $design (Status: $status, Time: ${test_elapsed}s)" >> "$status_file"
    
    # Create a completion marker file
    touch "$LOG_DIR/.${design}.complete"
    
    # Extract key metrics if successful
    if [[ "$status" == "Success" ]]; then
        local canonical_forms=$(grep "Unique canonical assignments:" "$log_file" | tail -1 | awk '{print $NF}')
        local best_cost=$(grep "Best Cost:" "$log_file" | tail -1 | awk '{print $NF}')
        
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] METRICS: $design, Canonical Forms: ${canonical_forms:-N/A}, Best Cost: ${best_cost:-N/A}" >> "$status_file"
    fi
}

# Function to count running jobs (child processes)
count_running_jobs() {
    local count=0
    for pid in "${PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            ((count++))
        fi
    done
    echo $count
}

# Create an array to store running process IDs
PIDS=()

# Counter for completed jobs
COMPLETED=0

# Run jobs in parallel up to MAX_PARALLEL_JOBS
for design in "${DESIGNS[@]}"; do
    # Check if we're at the maximum number of parallel jobs
    while [[ $(count_running_jobs) -ge $MAX_PARALLEL_JOBS ]]; do
        # Wait a bit and update completion info
        sleep 5
        
        # Update completed count by checking marker files
        current_completed=$(ls -1 "$LOG_DIR"/.*.complete 2>/dev/null | wc -l)
        
        # If we have new completions, print progress update
        if [[ $current_completed -gt $COMPLETED ]]; then
            echo -e "${GREEN}Progress:${NC} $current_completed/${#DESIGNS[@]} jobs completed, $(count_running_jobs) currently running"
            COMPLETED=$current_completed
        fi
    done
    
    # Remove any old completion marker
    rm -f "$LOG_DIR/.${design}.complete" 2>/dev/null
    
    # Start a new job in the background
    echo -e "${CYAN}Starting job for design:${NC} $design"
    run_test_case "$design" "$STATUS_FILE" &
    
    # Store the PID
    PIDS+=($!)
done

# Wait for all jobs to complete
echo -e "${BLUE}All jobs started. Waiting for completion...${NC}"

while [[ $(count_running_jobs) -gt 0 ]]; do
    # Update completed count
    current_completed=$(ls -1 "$LOG_DIR"/.*.complete 2>/dev/null | wc -l)
    
    # If we have new completions, print progress update
    if [[ $current_completed -gt $COMPLETED ]]; then
        echo -e "${GREEN}Progress:${NC} $current_completed/${#DESIGNS[@]} jobs completed, $(count_running_jobs) currently running"
        COMPLETED=$current_completed
    fi
    
    sleep 10
done

# Clean up completion markers
rm -f "$LOG_DIR"/.*.complete 2>/dev/null

# Calculate and display total elapsed time
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
HOURS=$((ELAPSED / 3600))
MINUTES=$(((ELAPSED % 3600) / 60))
SECONDS=$((ELAPSED % 60))

echo -e "\n${BLUE}=== Parallel Test Run Complete ===${NC}"

# Count results by analyzing log files
SUCCESS_COUNT=0
FAIL_COUNT=0

for design in "${DESIGNS[@]}"; do
    LOG_FILE="$LOG_DIR/${design}.log"
    
    if grep -q "Test completed with status: Success" "$LOG_FILE"; then
        ((SUCCESS_COUNT++))
    else
        ((FAIL_COUNT++))
    fi
done

echo -e "${GREEN}Successfully completed:${NC} $SUCCESS_COUNT/${#DESIGNS[@]} designs"
echo -e "${RED}Failed:${NC} $FAIL_COUNT/${#DESIGNS[@]} designs"
echo -e "${BLUE}Total time:${NC} ${HOURS}h ${MINUTES}m ${SECONDS}s"
echo -e "${BLUE}Log files are available in:${NC} $LOG_DIR/"

# Generate a summary file
SUMMARY_FILE="$LOG_DIR/summary.txt"
echo "Technology Enumeration Test Suite Summary (Parallel Run)" > "$SUMMARY_FILE"
echo "Date: $(date '+%Y-%m-%d %H:%M:%S')" >> "$SUMMARY_FILE"
echo "Technology Nodes: $TECH_NODES" >> "$SUMMARY_FILE"
echo "Max Partitions: $MAX_PARTITIONS" >> "$SUMMARY_FILE"
echo "Max Parallel Jobs: $MAX_PARALLEL_JOBS" >> "$SUMMARY_FILE"
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