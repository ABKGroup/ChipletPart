#!/bin/bash

# Example script to demonstrate the compositional partitioner

# Set colors for better output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}Compositional Partitioner Example - Using Real Test Cases${NC}"
echo

# Step 1: Verify the test cases exist
echo -e "${GREEN}Step 1: Verifying all test cases${NC}"
./compositional_partitioner.sh verify-test-cases
echo

# Step 2: Generate partitions for all base test cases
echo -e "${GREEN}Step 2: Generating partitions for all base test cases${NC}"
./compositional_partitioner.sh generate-partitions --tech 7nm --seed 42
echo

# Step 3: Evaluate compositional strategies for 48_2
echo -e "${GREEN}Step 3: Evaluating compositional strategies for 48_2${NC}"
./compositional_partitioner.sh compose-48_2 --tech 7nm --seed 42
echo

# Step 4: Evaluate compositional strategies for 48_4
echo -e "${GREEN}Step 4: Evaluating compositional strategies for 48_4${NC}"
./compositional_partitioner.sh compose-48_4 --tech 7nm --seed 42
echo

# Step 5: Evaluate compositional strategies for 48_8
echo -e "${GREEN}Step 5: Evaluating compositional strategies for 48_8${NC}"
./compositional_partitioner.sh compose-48_8 --tech 7nm --seed 42
echo

# Step 6: Evaluate compositional strategies for 48_16
echo -e "${GREEN}Step 6: Evaluating compositional strategies for 48_16${NC}"
./compositional_partitioner.sh compose-48_16 --tech 7nm --seed 42
echo

# Step 7: Summarize the results
echo -e "${BLUE}Compositional Partitioner Example - Summary${NC}"

# Check if the 48_2 evaluation results exist
if [ -f "results/compositional/48_2_evaluation/best_strategy.txt" ]; then
    best_strategy_48_2=$(cat "results/compositional/48_2_evaluation/best_strategy.txt")
    
    # Get the cost of the best strategy
    if [ "$best_strategy_48_2" == "direct" ]; then
        best_cost_48_2=$(cat "results/compositional/48_2_14_4_1600_1600_direct/objective_cost.txt")
    else
        best_cost_48_2=$(cat "results/compositional/48_2_evaluation/48_1_x2/objective_cost.txt")
    fi
    
    echo -e "${GREEN}Best strategy for 48_2: ${best_strategy_48_2} (Cost: ${best_cost_48_2})${NC}"
else
    echo -e "${YELLOW}No evaluation results found for 48_2${NC}"
fi

# Check if the 48_4 evaluation results exist
if [ -f "results/compositional/48_4_evaluation/best_strategy.txt" ]; then
    best_strategy_48_4=$(cat "results/compositional/48_4_evaluation/best_strategy.txt")
    
    # Display the best strategy and cost
    echo -e "${GREEN}Best strategy for 48_4: ${best_strategy_48_4}${NC}"
    if [ -f "results/compositional/48_4_evaluation/best_partition.txt" ]; then
        echo -e "${GREEN}Best partition file: results/compositional/48_4_evaluation/best_partition.txt${NC}"
    fi
else
    echo -e "${YELLOW}No evaluation results found for 48_4${NC}"
fi

# Check if the 48_8 evaluation results exist
if [ -f "results/compositional/48_8_evaluation/best_strategy.txt" ]; then
    best_strategy_48_8=$(cat "results/compositional/48_8_evaluation/best_strategy.txt")
    
    # Display the best strategy and cost
    echo -e "${GREEN}Best strategy for 48_8: ${best_strategy_48_8}${NC}"
    if [ -f "results/compositional/48_8_evaluation/best_partition.txt" ]; then
        echo -e "${GREEN}Best partition file: results/compositional/48_8_evaluation/best_partition.txt${NC}"
    fi
else
    echo -e "${YELLOW}No evaluation results found for 48_8${NC}"
fi

# Check if the 48_16 evaluation results exist
if [ -f "results/compositional/48_16_evaluation/best_strategy.txt" ]; then
    best_strategy_48_16=$(cat "results/compositional/48_16_evaluation/best_strategy.txt")
    
    # Display the best strategy and cost
    echo -e "${GREEN}Best strategy for 48_16: ${best_strategy_48_16}${NC}"
    if [ -f "results/compositional/48_16_evaluation/best_partition.txt" ]; then
        echo -e "${GREEN}Best partition file: results/compositional/48_16_evaluation/best_partition.txt${NC}"
    fi
else
    echo -e "${YELLOW}No evaluation results found for 48_16${NC}"
fi

echo
echo -e "${BLUE}Compositional partitioner example completed.${NC}"
echo -e "Results are available in the results/compositional directory." 