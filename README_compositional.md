# Compositional Partitioner for ChipletPart

This tool implements a hierarchical partitioning strategy that leverages the ChipletPart framework to produce optimized partitions for larger chip designs by composing partitions from smaller designs.

## Overview

The compositional partitioner addresses the challenge of partitioning large designs by:

1. Partitioning smaller test cases using the standard ChipletPart algorithm
2. Merging these partitions in various logical combinations
3. Comparing the results with direct partitioning
4. Selecting the best solution based on objective cost

## Features

- **Hierarchical Partitioning**: Leverage partitioning results from smaller designs to construct solutions for larger designs
- **Multiple Composition Strategies**: Evaluate different ways of combining smaller partitions
- **Automated Evaluation**: Compare different approaches and select the best one
- **Configurable Parameters**: Customize reach, separation, technology node, and random seed

## Test Cases

The system is designed to work with the following tile-based test cases:

- `48_1_14_4_1600_1600` - Base case with a single tile
- `48_2_14_4_1600_1600` - Design with 2 tiles
- `48_4_14_4_1600_1600` - Design with 4 tiles
- `48_8_14_4_1600_1600` - Design with 8 tiles

## Compositional Strategies

For each target test case, we evaluate multiple strategies:

### 48_2_14_4_1600_1600

1. Direct partitioning of 48_2
2. Composition: 48_1 x 2 (two instances of 48_1)

### 48_4_14_4_1600_1600

1. Direct partitioning of 48_4
2. Composition: 48_1 x 4 (four instances of 48_1)
3. Composition: 48_2 x 2 (two instances of 48_2)
4. Composition: 48_2 x 1 + 48_1 x 2 (one instance of 48_2 and two instances of 48_1)

### 48_8_14_4_1600_1600

1. Direct partitioning of 48_8
2. Composition: 48_1 x 8 (eight instances of 48_1)
3. Composition: 48_2 x 4 (four instances of 48_2)
4. Composition: 48_4 x 2 (two instances of 48_4)
5. Composition: 48_4 x 1 + 48_2 x 2 (one instance of 48_4 and two instances of 48_2)

## Usage

```bash
./compositional_partitioner.sh <command> [options]
```

### Commands

- `create-base-testcases` - Create the base test cases (48_1, 48_2, 48_4, 48_8)
- `generate-partitions` - Generate partitions for all base test cases
- `compose-48_2` - Evaluate compositional strategies for 48_2
- `compose-48_4` - Evaluate compositional strategies for 48_4
- `compose-48_8` - Evaluate compositional strategies for 48_8
- `evaluate-all` - Run all compositional evaluations

### Options

- `--reach <value>` - Specify reach value (default: 0.50)
- `--separation <value>` - Specify separation value (default: 0.25)
- `--tech <node>` - Specify tech node (default: 7nm)
- `--seed <value>` - Specify random seed (default: 42)
- `--help` - Display help message

### Examples

```bash
# Create base test cases
./compositional_partitioner.sh create-base-testcases

# Generate partitions for all base test cases
./compositional_partitioner.sh generate-partitions --tech 7nm

# Evaluate compositional strategies for 48_4
./compositional_partitioner.sh compose-48_4 --tech 7nm --seed 123

# Run all evaluations
./compositional_partitioner.sh evaluate-all
```

## Implementation Details

### Partition Merging Process

The partition merging process involves:

1. Reading the partition assignments from the source partition files
2. Adjusting vertex IDs to avoid conflicts between different partitions
3. Preserving the relative partition assignments within each source partition
4. Creating a consolidated partition file that maps the merged vertices to appropriate partitions

### Evaluation Process

For each target test case, we:

1. Run direct partitioning using ChipletPart
2. Generate all specified compositional strategies
3. Extract and compare the objective costs
4. Select the strategy with the lowest cost

## Technical Notes

- The current implementation simulates the merging process, as the full implementation would require:
  - Creating the test cases 48_2, 48_4, and 48_8 by duplicating and modifying the 48_1 test case
  - Implementing the logic to map block IDs and connections in the merged designs
  - Evaluating the merged partitions using the cost model

- In a full implementation, the tool would:
  1. Generate the netlist and block definitions for the larger test cases
  2. Create merged partitions by adapting the vertex IDs and preserving the partition assignments
  3. Evaluate the merged partitions using the cost model to get accurate objective costs
  4. Automatically select and apply the best strategy based on the evaluation results

## Future Work

- Implement the generation of larger test cases from the base case
- Develop the full partition merging logic to create valid merged partitions
- Integrate with the cost model to evaluate merged partitions
- Add support for more complex compositional strategies
- Extend to handle arbitrary design sizes and topologies 