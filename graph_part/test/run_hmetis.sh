#!/bin/bash

# Define common base path
base_path="../testcases"

#test_cases=("48_1_14_4_1600_1600")
test_cases=("mempool_group_100_100")

# Common parameters
reach="2"
separation="0.1"
tech="45nm"

# Create a directory for logs if it doesn't exist
log_dir="./logs"
mkdir -p $log_dir

for case_id in "${test_cases[@]}"; do
    # Iterate over different numbers of parts
    for num_parts in 2 3 4 8 16; do
        # Construct file paths
        #hgr="${base_path}/${case_id}/block_level_netlist_ws-${case_id}_hmetis.hgr"
        hgr="${base_path}/${case_id}/block_level_netlist_mempool_group_100_100_hmetis.hgr"
        for seed in 1 2 3 4 5; do
            echo "Running hmetis for case: $case_id with $num_parts parts and seed: $seed"
            cmd="./hmetis ${hgr} ${num_parts} 5 10 1 1 1 0 24 $seed"
            eval $cmd
            cmd="mv ${hgr}.part.${num_parts} ${hgr}.seed.${seed}.part.${num_parts}"
            eval $cmd
        done
    done
done

for case_id in "${test_cases[@]}"; do
    # Iterate over different numbers of parts
    for num_parts in 2 3 4 8 16; do
        # Construct file paths
        #hgr="${base_path}/${case_id}/block_level_netlist_ws-${case_id}.hgr"
        hgr="${base_path}/${case_id}/block_level_netlist_mempool_group_100_100.hgr"
        io_definitions="${base_path}/${case_id}/io_definitions.xml"
        layer_definitions="${base_path}/${case_id}/layer_definitions.xml"
        wafer_process_definitions="${base_path}/${case_id}/wafer_process_definitions.xml"
        assembly_process_definitions="${base_path}/${case_id}/assembly_process_definitions.xml"
        test_definitions="${base_path}/${case_id}/test_definitions.xml"
        #netlist_xml="${base_path}/${case_id}/block_level_netlist_ws-${case_id}.xml"
        netlist_xml="${base_path}/${case_id}/block_level_netlist_mempool_group_100_100.xml"
        #block_definitions="${base_path}/${case_id}/block_definitions_ws-${case_id}.txt"
        block_definitions="${base_path}/${case_id}/block_definitions_mempool_group_100_100.txt"

        for seed in 1 2 3 4 5; do
            echo "Running chipletPart for case: $case_id with $num_parts parts and seed: $seed"
            #part="${base_path}/${case_id}/block_level_netlist_ws-${case_id}_hmetis.hgr.seed.${seed}.part.${num_parts}"
            part="${base_path}/${case_id}/block_level_netlist_mempool_group_100_100_hmetis.hgr.seed.${seed}.part.${num_parts}"
            log_file="${log_dir}/${case_id}.hmetis.seed.${seed}.part.${num_parts}.log"
            cmd="../build/chipletPart $hgr $part $io_definitions $layer_definitions $wafer_process_definitions $assembly_process_definitions $test_definitions $netlist_xml $block_definitions $reach $separation $tech"
            echo "Running command for case: $case_id with $num_parts parts"
            echo "Command: $cmd" > $log_file
            eval $cmd >> $log_file 2>&1
        done
    done
done

echo "All test cases processed successfully."
