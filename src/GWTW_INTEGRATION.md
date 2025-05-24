# GWTW Floorplanner Integration

This document describes the integration of the Generalized-Where-The-Wind-Blows (GWTW) floorplanner that has replaced the previous simulated annealing (SA) based floorplanner.

## Changes Made

1. **Core Files Replaced**:
   - `floorplan.h` and `floorplan.cpp` have been replaced with the GWTW implementation.
   - The SACore class now includes a worker_id parameter for parallel coordination.

2. **Interface Updates**:
   - `FMRefiner::Floorplanner()` method has been updated to implement the GWTW algorithm.
   - `FMRefiner::RunSA()` and `FMRefiner::RunSASegment()` methods have been updated to call run() without parameters.

3. **New Features Added**:
   - GWTW parameters added as class members to `FMRefiner`.
   - Setter methods for GWTW parameters added to allow customization.
   - Performance tracking for floorplanning runtime added.

4. **Key GWTW Parameters**:
   - `gwtw_iter_` - Number of GWTW iterations (default: 2)
   - `gwtw_max_temp_` - Initial temperature (default: 100.0)
   - `gwtw_min_temp_` - Final temperature (default: 1e-12)
   - `gwtw_sync_freq_` - Synchronization frequency (default: 0.1)
   - `gwtw_top_k_` - Number of top solutions to propagate (default: 2)
   - `gwtw_temp_derate_factor_` - Temperature reduction factor (default: 1.0)
   - `gwtw_top_k_ratio_` - Distribution of workers for each top solution (default: {0.5, 0.5})

## Benefits of GWTW vs. Previous SA

1. **Improved Solution Quality**:
   - Multiple parallel workers explore different parts of the solution space.
   - Periodic synchronization of top solutions allows for faster convergence.
   - Temperature adjustment between iterations helps escape local minima.

2. **Better Parallelization**:
   - More sophisticated worker coordination through the GWTW algorithm.
   - Efficient propagation of promising solutions across workers.
   - Adjustable synchronization frequency for better resource utilization.

3. **Enhanced Configurability**:
   - More parameters to tune the floorplanning process.
   - Easier to adapt to different chiplet configurations.

## How to Use

The GWTW floorplanner maintains compatibility with the existing codebase. The same interface methods are used, but with the enhanced implementation.

### Example:

```cpp
// Configure GWTW parameters (optional - defaults are reasonable)
refiner.SetGWTWIterations(2);
refiner.SetGWTWMaxTemp(100.0);
refiner.SetGWTWTopK(2);

// Run floorplanner - same interface as before
auto result = refiner.RunFloorplanner(partition, hgraph, max_steps, perturbations, cooling_factor);

// Get results
bool is_valid = std::get<3>(result);
std::vector<float> aspect_ratios = std::get<0>(result);
std::vector<float> x_locations = std::get<1>(result);
std::vector<float> y_locations = std::get<2>(result);
```

## Testing

A test program `test/test_gwtw_floorplanner.cpp` has been added to verify the implementation. Run the test to ensure the GWTW floorplanner works correctly:

```
cd src/test
make
./test_gwtw_floorplanner
```

## Additional Notes

- The GWTW implementation is a more efficient approach to floorplanning that maintains the same input/output interfaces.
- For large designs, consider increasing `gwtw_iter_` and `gwtw_top_k_` for better quality at the cost of runtime.
- The `gwtw_sync_freq_` parameter controls the trade-off between exploration and exploitation - lower values allow more independent exploration. 