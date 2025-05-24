#!/bin/bash

# Compositional Partitioner for ChipletPart
# This script implements a hierarchical partitioning strategy by:
# 1. Partitioning smaller test cases
# 2. Merging these partitions in various combinations
# 3. Comparing the results with direct partitioning
# 4. Selecting the best solution based on objective cost

# Set colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Define base directory and other paths
BASE_DIR="$(pwd)"
BUILD_DIR="${BASE_DIR}/build"
EXECUTABLE="${BUILD_DIR}/bin/chipletPart"
RESULTS_DIR="${BASE_DIR}/results/compositional"
TEST_DATA_DIR="${BASE_DIR}/test_data"
TEMP_DIR="${BASE_DIR}/temp_compositional"

# Create necessary directories
mkdir -p "${RESULTS_DIR}"
mkdir -p "${TEMP_DIR}"

# Default parameters
DEFAULT_REACH="0.50"
DEFAULT_SEPARATION="0.25"
DEFAULT_TECH="7nm"
DEFAULT_SEED="42"

# Function to display usage information
function show_help {
    echo -e "${BLUE}Compositional Partitioner for ChipletPart${NC}"
    echo
    echo "Usage: $0 <command> [options]"
    echo
    echo "Commands:"
    echo "  verify-test-cases        Verify that all test cases exist and contain required files"
    echo "  generate-partitions      Generate partitions for all base test cases"
    echo "  compose-48_2             Evaluate compositional strategies for 48_2"
    echo "  compose-48_4             Evaluate compositional strategies for 48_4"
    echo "  compose-48_8             Evaluate compositional strategies for 48_8"
    echo "  compose-48_16            Evaluate compositional strategies for 48_16"
    echo "  evaluate-all             Run all compositional evaluations"
    echo
    echo "Options:"
    echo "  --reach <value>          Specify reach value (default: ${DEFAULT_REACH})"
    echo "  --separation <value>     Specify separation value (default: ${DEFAULT_SEPARATION})"
    echo "  --tech <node>            Specify tech node (default: ${DEFAULT_TECH})"
    echo "  --seed <value>           Specify random seed (default: ${DEFAULT_SEED})"
    echo "  --help                   Display this help message"
    echo
    echo "Example: $0 compose-48_4 --tech 7nm --seed 123"
}

# Function to create base test cases from the existing test case
function verify_test_cases {
    echo -e "${CYAN}Verifying existing test cases...${NC}"
    
    # Check if all required test cases exist
    local all_cases_exist=true
    for test_case in "48_1_14_4_1600_1600" "48_2_14_4_1600_1600" "48_4_14_4_1600_1600" "48_8_14_4_1600_1600" "48_16_14_4_1600_1600"; do
        if [ ! -d "${TEST_DATA_DIR}/${test_case}" ]; then
            echo -e "${RED}Error: Test case directory ${TEST_DATA_DIR}/${test_case} does not exist.${NC}"
            all_cases_exist=false
        else
            # Check if all required files exist
            local test_dir="${TEST_DATA_DIR}/${test_case}"
            for file in "io_definitions.xml" "layer_definitions.xml" "wafer_process_definitions.xml" \
                        "assembly_process_definitions.xml" "test_definitions.xml" \
                        "block_level_netlist.xml" "block_definitions.txt"; do
                if [ ! -f "${test_dir}/${file}" ]; then
                    echo -e "${RED}Error: File ${file} not found in ${test_dir}.${NC}"
                    all_cases_exist=false
                fi
            done
            echo -e "${GREEN}Test case ${test_case} verified.${NC}"
        fi
    done
    
    if [ "$all_cases_exist" = true ]; then
        echo -e "${GREEN}All test cases exist and contain required files.${NC}"
    else
        echo -e "${RED}Some test cases are missing or incomplete. Please ensure all test cases exist in ${TEST_DATA_DIR}.${NC}"
        exit 1
    fi
}

# Function to run the chipletPart executable on a test case
function run_chiplet_part {
    local test_case=$1
    local output_dir=$2
    local tech=$3
    local reach=$4
    local separation=$5
    local seed=$6
    
    echo -e "${BLUE}Running ChipletPart on ${test_case}...${NC}"
    
    # Define paths for the input files
    local test_dir="${TEST_DATA_DIR}/${test_case}"
    
    # Create output directory
    mkdir -p "${output_dir}"
    
    # Run the ChipletPart executable using the existing script
    echo -e "Running: ./run_chiplet_test.sh ${test_case} --tech ${tech} --reach ${reach} --separation ${separation} --seed ${seed}"
    
    ./run_chiplet_test.sh "${test_case}" --tech "${tech}" --reach "${reach}" --separation "${separation}" --seed "${seed}" | tee "${output_dir}/run.log"
    
    # Check if the execution was successful
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error: ChipletPart execution failed for ${test_case}${NC}"
        return 1
    fi
    
    # Copy partition results from the test data directory to the output directory
    local partition_file="${TEST_DATA_DIR}/${test_case}/block_level_netlist.xml.cpart.4"
    if [ -f "${partition_file}" ]; then
        cp "${partition_file}" "${output_dir}/partition.txt"
        echo -e "${GREEN}Found partition file: ${partition_file}${NC}"
    else
        echo -e "${YELLOW}Warning: Could not find expected partition file at ${partition_file}${NC}"
        # Look for other cpart files
        for cpart_file in ${TEST_DATA_DIR}/${test_case}/*.cpart.*; do
        if [ -f "$cpart_file" ]; then
                cp "$cpart_file" "${output_dir}/partition.txt"
            echo -e "${GREEN}Found partition file: $cpart_file${NC}"
                break
        fi
    done
    fi
    
    # Extract and store the objective cost from the log file
    local cost=$(grep "Objective Cost:" "${output_dir}/run.log" | tail -1 | awk '{print $NF}')
    if [ -n "$cost" ]; then
        echo "$cost" > "${output_dir}/objective_cost.txt"
        echo -e "${GREEN}ChipletPart completed for ${test_case}. Objective cost: ${cost}${NC}"
    else
        # Try another pattern that might be in the log
        cost=$(grep "Cost" "${output_dir}/run.log" | grep -o '[0-9]\+\.[0-9]\+' | tail -1)
        if [ -n "$cost" ]; then
            echo "$cost" > "${output_dir}/objective_cost.txt"
            echo -e "${GREEN}ChipletPart completed for ${test_case}. Objective cost: ${cost}${NC}"
        else
            echo -e "${YELLOW}Warning: Could not extract objective cost for ${test_case}${NC}"
            return 1
        fi
    fi
    
    return 0
}

# Function to generate partitions for all base test cases
function generate_partitions {
    echo -e "${CYAN}Generating partitions for base test cases...${NC}"
    
    local reach=${1:-$DEFAULT_REACH}
    local separation=${2:-$DEFAULT_SEPARATION}
    local tech=${3:-$DEFAULT_TECH}
    local seed=${4:-$DEFAULT_SEED}
    
    # Make sure the 48_1 test case is run first, since it's needed for the compositions
    run_chiplet_part "48_1_14_4_1600_1600" "${RESULTS_DIR}/48_1_14_4_1600_1600_direct" "$tech" "$reach" "$separation" "$seed"
    
    # Run ChipletPart on each remaining base test case
    for test_case in "48_2_14_4_1600_1600" "48_4_14_4_1600_1600" "48_8_14_4_1600_1600" "48_16_14_4_1600_1600"; do
        local output_dir="${RESULTS_DIR}/${test_case}_direct"
        
        # Skip if the test case directory doesn't exist
        if [ ! -d "${TEST_DATA_DIR}/${test_case}" ]; then
            echo -e "${YELLOW}Warning: Test case directory ${TEST_DATA_DIR}/${test_case} does not exist. Skipping.${NC}"
            continue
        fi
        
        run_chiplet_part "$test_case" "$output_dir" "$tech" "$reach" "$separation" "$seed"
    done
    
    echo -e "${GREEN}Partition generation completed.${NC}"
}

# Function to merge partitions from smaller test cases
function merge_partitions {
    echo -e "${BLUE}Merging partitions...${NC}"
    
    local output_file=$1
    shift
    local partition_files=("$@")
    
    # Check if all input files exist
    for file in "${partition_files[@]}"; do
        if [ ! -f "$file" ]; then
            echo -e "${RED}Error: Partition file not found: $file${NC}"
            return 1
        fi
    done
    
    # Use the Python utility to merge the partition files
    local partition_files_args=""
    for file in "${partition_files[@]}"; do
        partition_files_args+=" $file"
    done
    
    echo -e "Merging partitions: ${partition_files_args} into ${output_file}"
    
    ${BASE_DIR}/compositional_utils.py merge-partitions \
        --output-file "$output_file" \
        --partition-files $partition_files_args \
        --preserve-relative
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error: Failed to merge partition files.${NC}"
        return 1
    fi
    
    echo -e "${GREEN}Partitions merged into ${output_file}${NC}"
    
    return 0
}

# Function to extract cost from evaluation log
function extract_cost_from_log {
    local log_file=$1
    local cost=""
    
    # Try to extract from the evaluation output
    cost=$(grep "Partition evaluation cost:" "$log_file" | awk '{print $NF}')
    
    # If that fails, look for simulated cost
    if [ -z "$cost" ]; then
        cost=$(grep "Simulated cost:" "$log_file" | awk '{print $NF}')
    fi
    
    echo "$cost"
}

# Function to evaluate the composition of 48_2 test case
function compose_48_2 {
    echo -e "${CYAN}Evaluating compositional strategies for 48_2_14_4_1600_1600...${NC}"
    
    local reach=${1:-$DEFAULT_REACH}
    local separation=${2:-$DEFAULT_SEPARATION}
    local tech=${3:-$DEFAULT_TECH}
    local seed=${4:-$DEFAULT_SEED}
    
    # Create output directory for this evaluation
    local eval_dir="${RESULTS_DIR}/48_2_evaluation"
    mkdir -p "$eval_dir"
    
    # Strategy 1: Direct partitioning of 48_2
    local direct_result="${RESULTS_DIR}/48_2_14_4_1600_1600_direct"
    local direct_cost="N/A"
    
    if [ -f "${direct_result}/objective_cost.txt" ]; then
        direct_cost=$(cat "${direct_result}/objective_cost.txt")
    else
        echo -e "${YELLOW}Warning: Direct partitioning results for 48_2 not found. Running direct partitioning...${NC}"
        run_chiplet_part "48_2_14_4_1600_1600" "$direct_result" "$tech" "$reach" "$separation" "$seed"
        
        if [ -f "${direct_result}/objective_cost.txt" ]; then
            direct_cost=$(cat "${direct_result}/objective_cost.txt")
        else
            echo -e "${RED}Error: Failed to get direct partitioning cost for 48_2${NC}"
        fi
    fi
    
    # Strategy 2: Merge two instances of 48_1
    local merge_dir="${eval_dir}/48_1_x2"
    mkdir -p "$merge_dir"
    
    local result_48_1="${RESULTS_DIR}/48_1_14_4_1600_1600_direct"
    local merged_partition="${merge_dir}/partition.txt"
    
    if [ -f "${result_48_1}/partition.txt" ]; then
        # Merge two instances of the 48_1 partition
        merge_partitions "$merged_partition" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt"
        
        # Evaluate the merged partition using the ChipletPart cost model
        local test_case_dir="${TEST_DATA_DIR}/48_2_14_4_1600_1600"
        
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        # Extract and store the cost from the evaluation log
        local merged_cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$merged_cost" ]; then
        echo "$merged_cost" > "${merge_dir}/objective_cost.txt"
        else
            echo -e "${YELLOW}Warning: Could not extract cost from evaluation log.${NC}"
            local merged_cost="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_1 not found.${NC}"
        local merged_cost="N/A"
    fi
    
    # Compare the strategies and select the best one
    echo -e "${GREEN}Compositional evaluation for 48_2 completed.${NC}"
    echo -e "${BLUE}Results:${NC}"
    echo -e "  Direct partitioning: ${direct_cost}"
    
    if [ -f "${merge_dir}/objective_cost.txt" ]; then
        merged_cost=$(cat "${merge_dir}/objective_cost.txt")
    echo -e "  Merged 48_1 x2: ${merged_cost}"
    
        # Determine the better strategy
        if [ "$direct_cost" != "N/A" ] && [ "$merged_cost" != "N/A" ]; then
            if (( $(echo "$merged_cost < $direct_cost" | bc -l) )); then
                echo -e "${GREEN}The better strategy is: Merged 48_1 x2 (Cost: ${merged_cost})${NC}"
                echo "merged" > "${eval_dir}/best_strategy.txt"
                cp "${merged_partition}" "${eval_dir}/best_partition.txt"
            else
                echo -e "${GREEN}The better strategy is: Direct partitioning (Cost: ${direct_cost})${NC}"
                echo "direct" > "${eval_dir}/best_strategy.txt"
                cp "${direct_result}/partition.txt" "${eval_dir}/best_partition.txt"
            fi
        else
            echo -e "${YELLOW}Unable to determine the best strategy due to missing costs.${NC}"
        fi
    else
        echo -e "  Merged 48_1 x2: N/A (evaluation failed)"
    fi
    
    echo -e "Compositional evaluation results saved to ${eval_dir}."
}

# Function to evaluate the composition of 48_4 test case
function compose_48_4 {
    echo -e "${CYAN}Evaluating compositional strategies for 48_4_14_4_1600_1600...${NC}"
    
    local reach=${1:-$DEFAULT_REACH}
    local separation=${2:-$DEFAULT_SEPARATION}
    local tech=${3:-$DEFAULT_TECH}
    local seed=${4:-$DEFAULT_SEED}
    
    # Create output directory for this evaluation
    local eval_dir="${RESULTS_DIR}/48_4_evaluation"
    mkdir -p "$eval_dir"
    
    # Strategy 1: Direct partitioning of 48_4
    local direct_result="${RESULTS_DIR}/48_4_14_4_1600_1600_direct"
    local direct_cost="N/A"
    
    if [ -f "${direct_result}/objective_cost.txt" ]; then
        direct_cost=$(cat "${direct_result}/objective_cost.txt")
    else
        echo -e "${YELLOW}Warning: Direct partitioning results for 48_4 not found. Running direct partitioning...${NC}"
        run_chiplet_part "48_4_14_4_1600_1600" "$direct_result" "$tech" "$reach" "$separation" "$seed"
        
        if [ -f "${direct_result}/objective_cost.txt" ]; then
            direct_cost=$(cat "${direct_result}/objective_cost.txt")
        else
            echo -e "${RED}Error: Failed to get direct partitioning cost for 48_4${NC}"
        fi
    fi
    
    local test_case_dir="${TEST_DATA_DIR}/48_4_14_4_1600_1600"
    local result_48_1="${RESULTS_DIR}/48_1_14_4_1600_1600_direct"
    local result_48_2="${RESULTS_DIR}/48_2_14_4_1600_1600_direct"
    
    # Strategy 2: Merge four instances of 48_1
    local merge_dir1="${eval_dir}/48_1_x4"
    mkdir -p "$merge_dir1"
    local merged_partition1="${merge_dir1}/partition.txt"
    local merged_cost1="N/A"
    
    if [ -f "${result_48_1}/partition.txt" ]; then
        # Merge four instances of the 48_1 partition
        merge_partitions "$merged_partition1" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition1" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir1}/evaluation.log"
        
        # Extract and store the cost
        merged_cost1=$(extract_cost_from_log "${merge_dir1}/evaluation.log")
        if [ -n "$merged_cost1" ]; then
        echo "$merged_cost1" > "${merge_dir1}/objective_cost.txt"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_1 x4 strategy.${NC}"
            merged_cost1="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_1 not found.${NC}"
    fi
    
    # Strategy 3: Merge two instances of 48_2
    local merge_dir2="${eval_dir}/48_2_x2"
    mkdir -p "$merge_dir2"
    local merged_partition2="${merge_dir2}/partition.txt"
    local merged_cost2="N/A"
    
    if [ -f "${result_48_2}/partition.txt" ]; then
        # Merge two instances of the 48_2 partition
        merge_partitions "$merged_partition2" "${result_48_2}/partition.txt" "${result_48_2}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition2" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir2}/evaluation.log"
        
        # Extract and store the cost
        merged_cost2=$(extract_cost_from_log "${merge_dir2}/evaluation.log")
        if [ -n "$merged_cost2" ]; then
        echo "$merged_cost2" > "${merge_dir2}/objective_cost.txt"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_2 x2 strategy.${NC}"
            merged_cost2="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_2 not found.${NC}"
    fi
    
    # Strategy 4: Merge one instance of 48_2 and two instances of 48_1
    local merge_dir3="${eval_dir}/48_2_x1_48_1_x2"
    mkdir -p "$merge_dir3"
    local merged_partition3="${merge_dir3}/partition.txt"
    local merged_cost3="N/A"
    
    if [ -f "${result_48_1}/partition.txt" ] && [ -f "${result_48_2}/partition.txt" ]; then
        # Merge one 48_2 and two 48_1 partitions
        merge_partitions "$merged_partition3" "${result_48_2}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition3" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir3}/evaluation.log"
        
        # Extract and store the cost
        merged_cost3=$(extract_cost_from_log "${merge_dir3}/evaluation.log")
        if [ -n "$merged_cost3" ]; then
        echo "$merged_cost3" > "${merge_dir3}/objective_cost.txt"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_2 x1 + 48_1 x2 strategy.${NC}"
            merged_cost3="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_1 or 48_2 not found.${NC}"
    fi
    
    # Compare the strategies and select the best one
    echo -e "${GREEN}Compositional evaluation for 48_4 completed.${NC}"
    echo -e "${BLUE}Results:${NC}"
    echo -e "  Direct partitioning: ${direct_cost}"
    
    # Get all costs
    if [ -f "${merge_dir1}/objective_cost.txt" ]; then
        merged_cost1=$(cat "${merge_dir1}/objective_cost.txt")
    fi
    echo -e "  Merged 48_1 x4: ${merged_cost1}"
    
    if [ -f "${merge_dir2}/objective_cost.txt" ]; then
        merged_cost2=$(cat "${merge_dir2}/objective_cost.txt")
    fi
    echo -e "  Merged 48_2 x2: ${merged_cost2}"
    
    if [ -f "${merge_dir3}/objective_cost.txt" ]; then
        merged_cost3=$(cat "${merge_dir3}/objective_cost.txt")
    fi
    echo -e "  Merged 48_2 x1 + 48_1 x2: ${merged_cost3}"
    
    # Find the best strategy
    local best_cost="$direct_cost"
    local best_strategy="direct"
    local best_partition="${direct_result}/partition.txt"
    
    if [ "$merged_cost1" != "N/A" ] && (( $(echo "$merged_cost1 < $best_cost" | bc -l) )); then
        best_cost="$merged_cost1"
        best_strategy="48_1_x4"
        best_partition="$merged_partition1"
    fi
    
    if [ "$merged_cost2" != "N/A" ] && (( $(echo "$merged_cost2 < $best_cost" | bc -l) )); then
        best_cost="$merged_cost2"
        best_strategy="48_2_x2"
        best_partition="$merged_partition2"
    fi
    
    if [ "$merged_cost3" != "N/A" ] && (( $(echo "$merged_cost3 < $best_cost" | bc -l) )); then
        best_cost="$merged_cost3"
        best_strategy="48_2_x1_48_1_x2"
        best_partition="$merged_partition3"
    fi
    
    echo -e "${GREEN}The best strategy is: ${best_strategy} (Cost: ${best_cost})${NC}"
    echo "$best_strategy" > "${eval_dir}/best_strategy.txt"
    
    if [ -f "$best_partition" ]; then
        cp "$best_partition" "${eval_dir}/best_partition.txt"
    else
        echo -e "${YELLOW}Warning: Best partition file not found.${NC}"
    fi
    
    echo -e "Compositional evaluation results saved to ${eval_dir}."
}

# Function to evaluate the composition of 48_8 test case
function compose_48_8 {
    echo -e "${CYAN}Evaluating compositional strategies for 48_8_14_4_1600_1600...${NC}"
    
    local reach=${1:-$DEFAULT_REACH}
    local separation=${2:-$DEFAULT_SEPARATION}
    local tech=${3:-$DEFAULT_TECH}
    local seed=${4:-$DEFAULT_SEED}
    
    # Create output directory for this evaluation
    local eval_dir="${RESULTS_DIR}/48_8_evaluation"
    mkdir -p "$eval_dir"
    
    # Strategy 1: Direct partitioning of 48_8
    local direct_result="${RESULTS_DIR}/48_8_14_4_1600_1600_direct"
    local direct_cost="N/A"
    
    if [ -f "${direct_result}/objective_cost.txt" ]; then
        direct_cost=$(cat "${direct_result}/objective_cost.txt")
    else
        echo -e "${YELLOW}Warning: Direct partitioning results for 48_8 not found. Running direct partitioning...${NC}"
        run_chiplet_part "48_8_14_4_1600_1600" "$direct_result" "$tech" "$reach" "$separation" "$seed"
        
        if [ -f "${direct_result}/objective_cost.txt" ]; then
            direct_cost=$(cat "${direct_result}/objective_cost.txt")
        else
            echo -e "${RED}Error: Failed to get direct partitioning cost for 48_8${NC}"
        fi
    fi
    
    local test_case_dir="${TEST_DATA_DIR}/48_8_14_4_1600_1600"
    local result_48_1="${RESULTS_DIR}/48_1_14_4_1600_1600_direct"
    local result_48_2="${RESULTS_DIR}/48_2_14_4_1600_1600_direct"
    local result_48_4="${RESULTS_DIR}/48_4_14_4_1600_1600_direct"
    
    # Strategy 2: Merge eight instances of 48_1
    local merge_dir1="${eval_dir}/48_1_x8"
    mkdir -p "$merge_dir1"
    local merged_partition1="${merge_dir1}/partition.txt"
    local merged_cost1="N/A"
    
    if [ -f "${result_48_1}/partition.txt" ]; then
        # Merge eight instances of the 48_1 partition
        merge_partitions "$merged_partition1" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" "${result_48_1}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition1" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir1}/evaluation.log"
        
        # Extract and store the cost
        merged_cost1=$(extract_cost_from_log "${merge_dir1}/evaluation.log")
        if [ -n "$merged_cost1" ]; then
        echo "$merged_cost1" > "${merge_dir1}/objective_cost.txt"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_1 x8 strategy.${NC}"
            merged_cost1="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_1 not found.${NC}"
    fi
    
    # Strategy 3: Merge four instances of 48_2
    local merge_dir2="${eval_dir}/48_2_x4"
    mkdir -p "$merge_dir2"
    local merged_partition2="${merge_dir2}/partition.txt"
    local merged_cost2="N/A"
    
    if [ -f "${result_48_2}/partition.txt" ]; then
        # Merge four instances of the 48_2 partition
        merge_partitions "$merged_partition2" "${result_48_2}/partition.txt" "${result_48_2}/partition.txt" "${result_48_2}/partition.txt" "${result_48_2}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition2" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir2}/evaluation.log"
        
        # Extract and store the cost
        merged_cost2=$(extract_cost_from_log "${merge_dir2}/evaluation.log")
        if [ -n "$merged_cost2" ]; then
        echo "$merged_cost2" > "${merge_dir2}/objective_cost.txt"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_2 x4 strategy.${NC}"
            merged_cost2="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_2 not found.${NC}"
    fi
    
    # Strategy 4: Merge two instances of 48_4
    local merge_dir3="${eval_dir}/48_4_x2"
    mkdir -p "$merge_dir3"
    local merged_partition3="${merge_dir3}/partition.txt"
    local merged_cost3="N/A"
    
    if [ -f "${result_48_4}/partition.txt" ]; then
        # Merge two instances of the 48_4 partition
        merge_partitions "$merged_partition3" "${result_48_4}/partition.txt" "${result_48_4}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition3" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir3}/evaluation.log"
        
        # Extract and store the cost
        merged_cost3=$(extract_cost_from_log "${merge_dir3}/evaluation.log")
        if [ -n "$merged_cost3" ]; then
        echo "$merged_cost3" > "${merge_dir3}/objective_cost.txt"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_4 x2 strategy.${NC}"
            merged_cost3="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_4 not found.${NC}"
    fi
    
    # Strategy 5: Merge one instance of 48_4 and two instances of 48_2
    local merge_dir4="${eval_dir}/48_4_x1_48_2_x2"
    mkdir -p "$merge_dir4"
    local merged_partition4="${merge_dir4}/partition.txt"
    local merged_cost4="N/A"
    
    if [ -f "${result_48_4}/partition.txt" ] && [ -f "${result_48_2}/partition.txt" ]; then
        # Merge one 48_4 and two 48_2 partitions
        merge_partitions "$merged_partition4" "${result_48_4}/partition.txt" "${result_48_2}/partition.txt" "${result_48_2}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition4" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir4}/evaluation.log"
        
        # Extract and store the cost
        merged_cost4=$(extract_cost_from_log "${merge_dir4}/evaluation.log")
        if [ -n "$merged_cost4" ]; then
        echo "$merged_cost4" > "${merge_dir4}/objective_cost.txt"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_4 x1 + 48_2 x2 strategy.${NC}"
            merged_cost4="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_2 or 48_4 not found.${NC}"
    fi
    
    # Compare the strategies and select the best one
    echo -e "${GREEN}Compositional evaluation for 48_8 completed.${NC}"
    echo -e "${BLUE}Results:${NC}"
    echo -e "  Direct partitioning: ${direct_cost}"
    
    # Get all costs
    if [ -f "${merge_dir1}/objective_cost.txt" ]; then
        merged_cost1=$(cat "${merge_dir1}/objective_cost.txt")
    fi
    echo -e "  Merged 48_1 x8: ${merged_cost1}"
    
    if [ -f "${merge_dir2}/objective_cost.txt" ]; then
        merged_cost2=$(cat "${merge_dir2}/objective_cost.txt")
    fi
    echo -e "  Merged 48_2 x4: ${merged_cost2}"
    
    if [ -f "${merge_dir3}/objective_cost.txt" ]; then
        merged_cost3=$(cat "${merge_dir3}/objective_cost.txt")
    fi
    echo -e "  Merged 48_4 x2: ${merged_cost3}"
    
    if [ -f "${merge_dir4}/objective_cost.txt" ]; then
        merged_cost4=$(cat "${merge_dir4}/objective_cost.txt")
    fi
    echo -e "  Merged 48_4 x1 + 48_2 x2: ${merged_cost4}"
    
    # Find the best strategy
    local best_cost="$direct_cost"
    local best_strategy="direct"
    local best_partition="${direct_result}/partition.txt"
    
    if [ "$merged_cost1" != "N/A" ] && (( $(echo "$merged_cost1 < $best_cost" | bc -l) )); then
        best_cost="$merged_cost1"
        best_strategy="48_1_x8"
        best_partition="$merged_partition1"
    fi
    
    if [ "$merged_cost2" != "N/A" ] && (( $(echo "$merged_cost2 < $best_cost" | bc -l) )); then
        best_cost="$merged_cost2"
        best_strategy="48_2_x4"
        best_partition="$merged_partition2"
    fi
    
    if [ "$merged_cost3" != "N/A" ] && (( $(echo "$merged_cost3 < $best_cost" | bc -l) )); then
        best_cost="$merged_cost3"
        best_strategy="48_4_x2"
        best_partition="$merged_partition3"
    fi
    
    if [ "$merged_cost4" != "N/A" ] && (( $(echo "$merged_cost4 < $best_cost" | bc -l) )); then
        best_cost="$merged_cost4"
        best_strategy="48_4_x1_48_2_x2"
        best_partition="$merged_partition4"
    fi
    
    echo -e "${GREEN}The best strategy is: ${best_strategy} (Cost: ${best_cost})${NC}"
    echo "$best_strategy" > "${eval_dir}/best_strategy.txt"
    
    if [ -f "$best_partition" ]; then
        cp "$best_partition" "${eval_dir}/best_partition.txt"
    else
        echo -e "${YELLOW}Warning: Best partition file not found.${NC}"
    fi
    
    echo -e "Compositional evaluation results saved to ${eval_dir}."
}

# Function to evaluate the composition of 48_16 test case
function compose_48_16 {
    echo -e "${CYAN}Evaluating compositional strategies for 48_16_14_4_1600_1600...${NC}"
    
    local reach=${1:-$DEFAULT_REACH}
    local separation=${2:-$DEFAULT_SEPARATION}
    local tech=${3:-$DEFAULT_TECH}
    local seed=${4:-$DEFAULT_SEED}
    
    # Create output directory for this evaluation
    local eval_dir="${RESULTS_DIR}/48_16_evaluation"
    mkdir -p "$eval_dir"
    
    # Strategy 1: Direct partitioning of 48_16
    local direct_result="${RESULTS_DIR}/48_16_14_4_1600_1600_direct"
    local direct_cost="N/A"
    
    if [ -f "${direct_result}/objective_cost.txt" ]; then
        direct_cost=$(cat "${direct_result}/objective_cost.txt")
    else
        echo -e "${YELLOW}Warning: Direct partitioning results for 48_16 not found. Running direct partitioning...${NC}"
        run_chiplet_part "48_16_14_4_1600_1600" "$direct_result" "$tech" "$reach" "$separation" "$seed"
        
        if [ -f "${direct_result}/objective_cost.txt" ]; then
            direct_cost=$(cat "${direct_result}/objective_cost.txt")
        else
            echo -e "${RED}Error: Failed to get direct partitioning cost for 48_16${NC}"
        fi
    fi
    
    local test_case_dir="${TEST_DATA_DIR}/48_16_14_4_1600_1600"
    local result_48_1="${RESULTS_DIR}/48_1_14_4_1600_1600_direct"
    local result_48_2="${RESULTS_DIR}/48_2_14_4_1600_1600_direct"
    local result_48_4="${RESULTS_DIR}/48_4_14_4_1600_1600_direct"
    local result_48_8="${RESULTS_DIR}/48_8_14_4_1600_1600_direct"
    
    # Array to store all strategy results
    declare -A strategies
    
    # Strategy 2: Merge sixteen instances of 48_1
    if [ -f "${result_48_1}/partition.txt" ]; then
        local merge_dir="${eval_dir}/48_1_x16"
        mkdir -p "$merge_dir"
        local merged_partition="${merge_dir}/partition.txt"
        
        echo -e "${GREEN}Evaluating strategy: 48_1 x16${NC}"
        merge_partitions "$merged_partition" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        local cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$cost" ]; then
            echo "$cost" > "${merge_dir}/objective_cost.txt"
            strategies["48_1_x16"]="$cost"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_1 x16 strategy.${NC}"
            strategies["48_1_x16"]="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_1 not found.${NC}"
        strategies["48_1_x16"]="N/A"
    fi
    
    # Strategy 3: Merge eight instances of 48_2
    if [ -f "${result_48_2}/partition.txt" ]; then
        local merge_dir="${eval_dir}/48_2_x8"
        mkdir -p "$merge_dir"
        local merged_partition="${merge_dir}/partition.txt"
        
        echo -e "${GREEN}Evaluating strategy: 48_2 x8${NC}"
        merge_partitions "$merged_partition" \
            "${result_48_2}/partition.txt" "${result_48_2}/partition.txt" \
            "${result_48_2}/partition.txt" "${result_48_2}/partition.txt" \
            "${result_48_2}/partition.txt" "${result_48_2}/partition.txt" \
            "${result_48_2}/partition.txt" "${result_48_2}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        local cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$cost" ]; then
            echo "$cost" > "${merge_dir}/objective_cost.txt"
            strategies["48_2_x8"]="$cost"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_2 x8 strategy.${NC}"
            strategies["48_2_x8"]="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_2 not found.${NC}"
        strategies["48_2_x8"]="N/A"
    fi
    
    # Strategy 4: Merge four instances of 48_4
    if [ -f "${result_48_4}/partition.txt" ]; then
        local merge_dir="${eval_dir}/48_4_x4"
        mkdir -p "$merge_dir"
        local merged_partition="${merge_dir}/partition.txt"
        
        echo -e "${GREEN}Evaluating strategy: 48_4 x4${NC}"
        merge_partitions "$merged_partition" \
            "${result_48_4}/partition.txt" "${result_48_4}/partition.txt" \
            "${result_48_4}/partition.txt" "${result_48_4}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        local cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$cost" ]; then
            echo "$cost" > "${merge_dir}/objective_cost.txt"
            strategies["48_4_x4"]="$cost"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_4 x4 strategy.${NC}"
            strategies["48_4_x4"]="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_4 not found.${NC}"
        strategies["48_4_x4"]="N/A"
    fi
    
    # Strategy 5: Merge two instances of 48_8
    if [ -f "${result_48_8}/partition.txt" ]; then
        local merge_dir="${eval_dir}/48_8_x2"
        mkdir -p "$merge_dir"
        local merged_partition="${merge_dir}/partition.txt"
        
        echo -e "${GREEN}Evaluating strategy: 48_8 x2${NC}"
        merge_partitions "$merged_partition" \
            "${result_48_8}/partition.txt" "${result_48_8}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        local cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$cost" ]; then
            echo "$cost" > "${merge_dir}/objective_cost.txt"
            strategies["48_8_x2"]="$cost"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_8 x2 strategy.${NC}"
            strategies["48_8_x2"]="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_8 not found.${NC}"
        strategies["48_8_x2"]="N/A"
    fi
    
    # Strategy 6: Merge one 48_8 and eight 48_1 instances
    if [ -f "${result_48_8}/partition.txt" ] && [ -f "${result_48_1}/partition.txt" ]; then
        local merge_dir="${eval_dir}/48_8_x1_48_1_x8"
        mkdir -p "$merge_dir"
        local merged_partition="${merge_dir}/partition.txt"
        
        echo -e "${GREEN}Evaluating strategy: 48_8 x1 + 48_1 x8${NC}"
        merge_partitions "$merged_partition" \
            "${result_48_8}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        local cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$cost" ]; then
            echo "$cost" > "${merge_dir}/objective_cost.txt"
            strategies["48_8_x1_48_1_x8"]="$cost"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_8 x1 + 48_1 x8 strategy.${NC}"
            strategies["48_8_x1_48_1_x8"]="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_8 or 48_1 not found.${NC}"
        strategies["48_8_x1_48_1_x8"]="N/A"
    fi
    
    # Strategy 7: Merge one 48_8, one 48_4, and two 48_2 instances
    if [ -f "${result_48_8}/partition.txt" ] && [ -f "${result_48_4}/partition.txt" ] && [ -f "${result_48_2}/partition.txt" ]; then
        local merge_dir="${eval_dir}/48_8_x1_48_4_x1_48_2_x2"
        mkdir -p "$merge_dir"
        local merged_partition="${merge_dir}/partition.txt"
        
        echo -e "${GREEN}Evaluating strategy: 48_8 x1 + 48_4 x1 + 48_2 x2${NC}"
        merge_partitions "$merged_partition" \
            "${result_48_8}/partition.txt" \
            "${result_48_4}/partition.txt" \
            "${result_48_2}/partition.txt" "${result_48_2}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        local cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$cost" ]; then
            echo "$cost" > "${merge_dir}/objective_cost.txt"
            strategies["48_8_x1_48_4_x1_48_2_x2"]="$cost"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_8 x1 + 48_4 x1 + 48_2 x2 strategy.${NC}"
            strategies["48_8_x1_48_4_x1_48_2_x2"]="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_8, 48_4, or 48_2 not found.${NC}"
        strategies["48_8_x1_48_4_x1_48_2_x2"]="N/A"
    fi
    
    # Strategy 8: Merge two 48_4 and eight 48_1 instances
    if [ -f "${result_48_4}/partition.txt" ] && [ -f "${result_48_1}/partition.txt" ]; then
        local merge_dir="${eval_dir}/48_4_x2_48_1_x8"
        mkdir -p "$merge_dir"
        local merged_partition="${merge_dir}/partition.txt"
        
        echo -e "${GREEN}Evaluating strategy: 48_4 x2 + 48_1 x8${NC}"
        merge_partitions "$merged_partition" \
            "${result_48_4}/partition.txt" "${result_48_4}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        local cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$cost" ]; then
            echo "$cost" > "${merge_dir}/objective_cost.txt"
            strategies["48_4_x2_48_1_x8"]="$cost"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_4 x2 + 48_1 x8 strategy.${NC}"
            strategies["48_4_x2_48_1_x8"]="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_4 or 48_1 not found.${NC}"
        strategies["48_4_x2_48_1_x8"]="N/A"
    fi
    
    # Strategy 9: Merge four 48_2 and eight 48_1 instances
    if [ -f "${result_48_2}/partition.txt" ] && [ -f "${result_48_1}/partition.txt" ]; then
        local merge_dir="${eval_dir}/48_2_x4_48_1_x8"
        mkdir -p "$merge_dir"
        local merged_partition="${merge_dir}/partition.txt"
        
        echo -e "${GREEN}Evaluating strategy: 48_2 x4 + 48_1 x8${NC}"
        merge_partitions "$merged_partition" \
            "${result_48_2}/partition.txt" "${result_48_2}/partition.txt" \
            "${result_48_2}/partition.txt" "${result_48_2}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt" \
            "${result_48_1}/partition.txt" "${result_48_1}/partition.txt"
        
        # Evaluate the merged partition
        ${BASE_DIR}/compositional_utils.py evaluate-partition \
            --test-case-dir "$test_case_dir" \
            --partition-file "$merged_partition" \
            --tech "$tech" \
            --reach "$reach" \
            --separation "$separation" \
            --seed "$seed" | tee "${merge_dir}/evaluation.log"
        
        local cost=$(extract_cost_from_log "${merge_dir}/evaluation.log")
        if [ -n "$cost" ]; then
            echo "$cost" > "${merge_dir}/objective_cost.txt"
            strategies["48_2_x4_48_1_x8"]="$cost"
        else
            echo -e "${YELLOW}Warning: Could not extract cost for 48_2 x4 + 48_1 x8 strategy.${NC}"
            strategies["48_2_x4_48_1_x8"]="N/A"
        fi
    else
        echo -e "${YELLOW}Warning: Partition results for 48_2 or 48_1 not found.${NC}"
        strategies["48_2_x4_48_1_x8"]="N/A"
    fi
    
    # Compare the strategies and select the best one
    echo -e "${GREEN}Compositional evaluation for 48_16 completed.${NC}"
    echo -e "${BLUE}Results:${NC}"
    
    # Add direct partitioning to strategies
    strategies["direct"]="$direct_cost"
    
    # Output all strategies and their costs
    echo -e "  Direct partitioning: ${direct_cost}"
    echo -e "  Merged 48_1 x16: ${strategies["48_1_x16"]}"
    echo -e "  Merged 48_2 x8: ${strategies["48_2_x8"]}"
    echo -e "  Merged 48_4 x4: ${strategies["48_4_x4"]}"
    echo -e "  Merged 48_8 x2: ${strategies["48_8_x2"]}"
    echo -e "  Merged 48_8 x1 + 48_1 x8: ${strategies["48_8_x1_48_1_x8"]}"
    echo -e "  Merged 48_8 x1 + 48_4 x1 + 48_2 x2: ${strategies["48_8_x1_48_4_x1_48_2_x2"]}"
    echo -e "  Merged 48_4 x2 + 48_1 x8: ${strategies["48_4_x2_48_1_x8"]}"
    echo -e "  Merged 48_2 x4 + 48_1 x8: ${strategies["48_2_x4_48_1_x8"]}"
    
    # Find the best strategy
    local best_cost="$direct_cost"
    local best_strategy="direct"
    local best_partition="${direct_result}/partition.txt"
    
    for strategy in "48_1_x16" "48_2_x8" "48_4_x4" "48_8_x2" "48_8_x1_48_1_x8" "48_8_x1_48_4_x1_48_2_x2" "48_4_x2_48_1_x8" "48_2_x4_48_1_x8"; do
        local current_cost="${strategies[$strategy]}"
        local strategy_dir="${eval_dir}/${strategy}"
        local strategy_partition="${strategy_dir}/partition.txt"
        
        if [ "$current_cost" != "N/A" ] && (( $(echo "$current_cost < $best_cost" | bc -l) )); then
            best_cost="$current_cost"
            best_strategy="$strategy"
            best_partition="$strategy_partition"
        fi
    done
    
    echo -e "${GREEN}The best strategy is: ${best_strategy} (Cost: ${best_cost})${NC}"
    echo "$best_strategy" > "${eval_dir}/best_strategy.txt"
    
    if [ -f "$best_partition" ]; then
        cp "$best_partition" "${eval_dir}/best_partition.txt"
    else
        echo -e "${YELLOW}Warning: Best partition file not found.${NC}"
    fi
    
    echo -e "Compositional evaluation results saved to ${eval_dir}."
}

# Function to run all compositional evaluations
function evaluate_all {
    echo -e "${CYAN}Running all compositional evaluations...${NC}"
    
    local reach=${1:-$DEFAULT_REACH}
    local separation=${2:-$DEFAULT_SEPARATION}
    local tech=${3:-$DEFAULT_TECH}
    local seed=${4:-$DEFAULT_SEED}
    
    # Verify test cases exist before proceeding
    verify_test_cases
    
    # Generate partitions for all base test cases
    generate_partitions "$reach" "$separation" "$tech" "$seed"
    
    # Evaluate compositional strategies for each target test case
    compose_48_2 "$reach" "$separation" "$tech" "$seed"
    compose_48_4 "$reach" "$separation" "$tech" "$seed"
    compose_48_8 "$reach" "$separation" "$tech" "$seed"
    compose_48_16 "$reach" "$separation" "$tech" "$seed"
    
    echo -e "${GREEN}All compositional evaluations completed.${NC}"
}

# Main script logic
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

# Parse command line arguments
COMMAND=$1
shift

# Parse options
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

# Execute the requested command
case "$COMMAND" in
    verify-test-cases)
        verify_test_cases
        ;;
    create-base-testcases)
        verify_test_cases  # For backward compatibility
        ;;
    generate-partitions)
        generate_partitions "$DEFAULT_REACH" "$DEFAULT_SEPARATION" "$DEFAULT_TECH" "$DEFAULT_SEED"
        ;;
    compose-48_2)
        compose_48_2 "$DEFAULT_REACH" "$DEFAULT_SEPARATION" "$DEFAULT_TECH" "$DEFAULT_SEED"
        ;;
    compose-48_4)
        compose_48_4 "$DEFAULT_REACH" "$DEFAULT_SEPARATION" "$DEFAULT_TECH" "$DEFAULT_SEED"
        ;;
    compose-48_8)
        compose_48_8 "$DEFAULT_REACH" "$DEFAULT_SEPARATION" "$DEFAULT_TECH" "$DEFAULT_SEED"
        ;;
    compose-48_16)
        compose_48_16 "$DEFAULT_REACH" "$DEFAULT_SEPARATION" "$DEFAULT_TECH" "$DEFAULT_SEED"
        ;;
    evaluate-all)
        evaluate_all "$DEFAULT_REACH" "$DEFAULT_SEPARATION" "$DEFAULT_TECH" "$DEFAULT_SEED"
        ;;
    *)
        echo -e "${RED}Error: Unknown command: $COMMAND${NC}"
        show_help
        exit 1
        ;;
esac

exit 0 