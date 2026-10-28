# GWTW Floorplanner Integration

This document describes the integration of the Go-With-The-Winners (GWTW) floorplanner that has replaced the previous simulated annealing (SA) based floorplanner.

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

4. **Key GWTW Parameters** (see `ChipletRefiner::Floorplanner`):
   - `gwtw_iter_` - Number of GWTW segments; after each segment (except the
     last) the workers are ranked and the winners are copied onto the rest
     (default: 2)
   - `gwtw_top_k_` - Number of top solutions to propagate (default: 2)
   - `gwtw_top_k_ratio_` - Share of the non-winning workers assigned to each
     winner (default: {0.5, 0.5})
   - `gwtw_temp_derate_factor_` - Multiplier applied to every worker's
     temperature after a synchronisation; 1.0 = unchanged (default: 1.0)
   - `gwtw_min_temp_` - Floor for the derated temperature (default: 1e-12)
   - `gwtw_max_temp_`, `gwtw_sync_freq_` - currently unused (the SA starts at
     `SACore::init_temperature_` and synchronises `gwtw_iter_` times)

   Workers are independent `SACore` instances with their own RNG and cooling
   rate (spread between `min_cooling_rate_` and `max_cooling_rate_`); a
   segment runs them in parallel with OpenMP when not already inside a
   parallel region. Synchronisation copies the sequence pair and chiplet
   shapes (`SACore::adoptSolution`), ranking valid solutions first and then
   by normalised cost.

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
