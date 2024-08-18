#!/bin/bash

# Define common base path
base_path="../testcases"

# Array of test case identifiers
test_cases=("3_4_14_4_400_1" "1_4_14_4_400_1")

# Common parameters
reach="2"
separation="0.1"
tech="45nm"

# Create a directory for logs if it doesn't exist
log_dir="./logs"
mkdir -p $log_dir

# Loop over test cases
for case_id in "${test_cases[@]}"; do
    # Construct file paths
    hgr="${base_path}/${case_id}/block_level_netlist_ws-${case_id}.hgr"
    io_definitions="${base_path}/${case_id}/io_definitions.xml"
    layer_definitions="${base_path}/${case_id}/layer_definitions.xml"
    wafer_process_definitions="${base_path}/${case_id}/wafer_process_definitions.xml"
    assembly_process_definitions="${base_path}/${case_id}/assembly_process_definitions.xml"
    test_definitions="${base_path}/${case_id}/test_definitions.xml"
    netlist_xml="${base_path}/${case_id}/block_level_netlist_ws-${case_id}.xml"
    block_definitions="${base_path}/${case_id}/block_definitions_ws-${case_id}.txt"

    # Log file path
    log_file="${log_dir}/${case_id}.log"

    # Build the command
    cmd="../build/chipletPart $hgr $io_definitions $layer_definitions $wafer_process_definitions $assembly_process_definitions $test_definitions $netlist_xml $block_definitions $reach $separation $tech"

    # Execute the command and redirect both stdout and stderr to log file
    echo "Running command for case: $case_id"
    echo "Command: $cmd" > $log_file
    $cmd >> $log_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error encountered with test case $case_id. Check log file $log_file for details."
        exit 1
    fi
done

echo "All test cases processed successfully."
