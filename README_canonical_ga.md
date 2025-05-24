# Canonical Genetic Algorithm for Technology Assignment

This document describes the Canonical Genetic Algorithm (CanonicalGA) implementation for chiplet technology assignment in ChipletPart.

## Current Status

The Canonical Genetic Algorithm is now fully implemented and operational! The string creation bug (`basic_string::_M_create` error) has been fixed by implementing safer string handling throughout the codebase. The fix includes:

1. Safe string copying using c_str() to create proper memory-managed string copies
2. Pre-allocation of memory with reserve() to avoid reallocation failures
3. Extra validation for string operations and proper error handling
4. Guards against empty strings and null references

The full CanonicalGA implementation includes:
- Advanced genetic algorithm that optimizes technology assignments for chiplets
- Canonicalization of technology assignments to avoid redundant evaluations 
- Fitness caching for efficient evaluation and faster convergence
- Customizable genetic operators (crossover, mutation)
- Support for variable numbers of chiplets
- Integration with cost models for full evaluation

## Using the Canonical GA

You can run the canonical GA mode using the provided scripts:

```bash
# Using the test script with a specific test case
./run_chiplet_test.sh ga100 --canonical-ga --tech-nodes 7nm,14nm,28nm

# Using the dedicated canonical GA script
./run_canonical_ga.sh --tech-nodes 7nm,14nm,28nm
```

You can customize the genetic algorithm parameters:
```bash
./run_canonical_ga.sh --tech-nodes 7nm,14nm,28nm --generations 50 --population 50
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