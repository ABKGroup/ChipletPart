#!/bin/bash

# Set colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Define base directories and files
BASE_DIR="$(pwd)"
BUILD_DIR="${BASE_DIR}/build"
EXECUTABLE="${BUILD_DIR}/bin/chipletPart"
TEST_DATA_PARENT="${BASE_DIR}/test_data"
RESULTS_DIR="${BASE_DIR}/results"
JOBS_FILE="${BASE_DIR}/genetic_jobs.txt"

# Default parameters
DEFAULT_REACH="0.50"
DEFAULT_SEPARATION="0.25"
DEFAULT_SEED="42"
DEFAULT_GENERATIONS="50"
DEFAULT_POPULATION="100"
TECH_NODES="7nm 10nm 45nm"  # Specified tech nodes

# Print header
echo -e "${BLUE}Generating genetic partitioning jobs with tech nodes: ${TECH_NODES}${NC}"

# Create results directory if it doesn't exist
mkdir -p "${RESULTS_DIR}"

# Clear jobs file
> "${JOBS_FILE}"

# Loop through all test cases in the test_data directory
for test_case_dir in "${TEST_DATA_PARENT}"/*/ ; do
    test_case_dir=${test_case_dir%/}  # Remove trailing slash
    test_case_name=$(basename "${test_case_dir}")
    
    # Define input files for this test case
    IO="${test_case_dir}/io_definitions.xml"
    LAYER="${test_case_dir}/layer_definitions.xml"
    WAFER="${test_case_dir}/wafer_process_definitions.xml"
    ASSEMBLY="${test_case_dir}/assembly_process_definitions.xml"
    TEST="${test_case_dir}/test_definitions.xml"
    NETLIST="${test_case_dir}/block_level_netlist.xml"
    BLOCKS="${test_case_dir}/block_definitions.txt"
    
    # Check that all required files exist
    all_files_exist=true
    for file in "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS"; do
        if [ ! -f "$file" ]; then
            echo -e "${RED}Warning: File not found: $file${NC}"
            echo -e "${RED}Skipping test case: ${test_case_name}${NC}"
            all_files_exist=false
            break
        fi
    done
    
    # Skip this test case if any files are missing
    if [ "$all_files_exist" = false ]; then
        continue
    fi
    
    # Define log file for this test case
    LOG_FILE="${RESULTS_DIR}/${test_case_name}_genetic.log"
    
    # Build the command for genetic partitioning without extra quotations
    CMD="${EXECUTABLE} ${IO} ${LAYER} ${WAFER} ${ASSEMBLY} ${TEST} ${NETLIST} ${BLOCKS} ${DEFAULT_REACH} ${DEFAULT_SEPARATION} --genetic-tech-part --tech-nodes ${TECH_NODES} --generations ${DEFAULT_GENERATIONS} --population ${DEFAULT_POPULATION} --seed ${DEFAULT_SEED} 2>&1 | tee ${LOG_FILE}"
    
    # Add the command to the jobs file
    echo "$CMD" >> "${JOBS_FILE}"
    
    echo -e "${GREEN}Added job for test case: ${test_case_name}${NC}"
done

echo -e "${GREEN}All genetic partitioning jobs have been written to: ${JOBS_FILE}${NC}"
echo -e "${YELLOW}To run all jobs sequentially: bash ${JOBS_FILE}${NC}"
echo -e "${YELLOW}To run jobs in parallel: cat ${JOBS_FILE} | parallel -j <num_cores> bash -c {}${NC}"

# Make the jobs file executable
chmod +x "${JOBS_FILE}"

echo -e "${BLUE}Done!${NC}"
