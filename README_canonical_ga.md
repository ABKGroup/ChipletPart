# Canonical Genetic Algorithm for Technology Assignment

This document describes the Canonical Genetic Algorithm (CanonicalGA) implementation for chiplet technology assignment in ChipletPart.

## When to use it

The GA searches over *canonical* technology assignments: for `T` technologies and at most `P` chiplets the
number of distinct assignments is small (164 for `T = 3, P = 8`; 5 for `T = 2, P = 2`). With the default
`--generations 50 --population 50` the GA performs up to 2,500 evaluations (fitness caching removes exact
repeats), i.e. far more than the size of the space it searches. For `T·P` of this order prefer

```bash
./run_chiplet_test.sh <case> --tech-enum --tech-nodes 7nm,10nm,14nm --max-partitions 8
```

which evaluates every canonical assignment exactly once and is deterministic. The GA is the right tool when the
number of technologies or chiplets makes enumeration impractical (hundreds of thousands of canonical forms), or
when you want to co-optimise the partition itself (`--genetic`, see `GeneticTechPartitioner`).

## Features

- Canonicalization of technology assignments to avoid redundant evaluations
- Fitness caching
- Customizable genetic operators (crossover, mutation)
- Support for variable numbers of chiplets
- Integration with the cost model for full evaluation

## Using the Canonical GA

You can run the canonical GA mode using the provided scripts:

```bash
# Using the test script with a specific test case
./run_chiplet_test.sh ga100 --canonical-ga --tech-nodes 7nm,10nm,14nm

# Using the dedicated canonical GA script (default test case: ga100)
./run_canonical_ga.sh --test-case ga100 --tech-nodes 7nm,10nm,14nm
```

You can customize the genetic algorithm parameters:
```bash
./run_canonical_ga.sh --tech-nodes 7nm,10nm,14nm --generations 50 --population 50
```

## Output Files

The canonical GA generates the following output files:

- `canonical_ga_result.parts.N` - Partition assignments (where N is the number of chiplets)
- `canonical_ga_result.techs.N` - Technology assignments for each chiplet
- `canonical_ga_result.summary.txt` - Summary of the partition and technology assignments

## Known Issues

- **String creation error**: FIXED - The `basic_string::_M_create` error that was previously occurring in the CanonicalGA implementation has been fixed by implementing safer string handling throughout the codebase.

## Future Work

- Add detailed statistics and visualizations for the genetic algorithm progress
- Improve the integration with the cost models for technology evaluation
- Expand the test suite with more comprehensive tests for the genetic algorithm
- Add support for additional optimization criteria and constraints 