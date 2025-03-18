#include "design/Layer.hpp"
#include "design/WaferProcess.hpp"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <cmath>

// Include OpenMP headers conditionally
#if defined(ENABLE_OPENMP) && !defined(DISABLE_OPENMP) && defined(_OPENMP)
#include <omp.h>
#endif

namespace design {

Layer::Layer(
    const String& name,
    const String& active,
    FloatType cost_per_mm2,
    FloatType transistor_density,
    FloatType defect_density,
    FloatType critical_area_ratio,
    FloatType clustering_factor,
    FloatType litho_percent,
    FloatType mask_cost,
    FloatType stitching_yield,
    BoolType static_value)
    : static_(false)  // Initialize static_ to false first
{
    // Initialize all properties
    name_ = name;
    
    // Convert active string to bool
    if (!active.empty()) {
        String lowerValue = active;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        active_ = (lowerValue == "true");
    } else {
        active_ = false;
    }
    
    cost_per_mm2_ = cost_per_mm2;
    transistor_density_ = transistor_density;
    defect_density_ = defect_density;
    critical_area_ratio_ = critical_area_ratio;
    clustering_factor_ = clustering_factor;
    litho_percent_ = litho_percent;
    mask_cost_ = mask_cost;
    stitching_yield_ = stitching_yield;
    
    // Set static value last
    static_ = static_value;
    
    // If not fully defined, set to non-static
    if (!LayerFullyDefined()) {
        std::cout << "Warning: Layer not fully defined. Setting non-static." << std::endl;
        static_ = false;
        std::cout << ToString() << std::endl;
    }
}

String Layer::GetName() const {
    return name_;
}

IntType Layer::SetName(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Layer name must be a string." << std::endl;
            return 1;
        } else {
            name_ = value;
            return 0;
        }
    }
}

BoolType Layer::GetActive() const {
    return active_;
}

IntType Layer::SetActive(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Active must be a string. (True or False)" << std::endl;
            return 1;
        } else {
            String lowerValue = value;
            std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            
            if (lowerValue == "true") {
                active_ = true;
            } else {
                active_ = false;
            }
            return 0;
        }
    }
}

FloatType Layer::GetCostPerMm2() const {
    return cost_per_mm2_;
}

IntType Layer::SetCostPerMm2(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Cost per mm^2 must be nonnegative." << std::endl;
            return 1;
        } else {
            cost_per_mm2_ = value;
            return 0;
        }
    }
}

FloatType Layer::GetTransistorDensity() const {
    return transistor_density_;
}

IntType Layer::SetTransistorDensity(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Transistor density must be nonnegative." << std::endl;
            return 1;
        } else {
            transistor_density_ = value;
            return 0;
        }
    }
}

FloatType Layer::GetGatesPerMm2() const {
    // Transistor density is in million transistors per mm^2. 
    // Divide by 4 (assuming 4-transistor NAND and NOR gates) to get gates per mm^2.
    return transistor_density_ * 1e6 / 4;
}

FloatType Layer::GetDefectDensity() const {
    return defect_density_;
}

IntType Layer::SetDefectDensity(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Defect density must be nonnegative." << std::endl;
            return 1;
        } else {
            defect_density_ = value;
            return 0;
        }
    }
}

FloatType Layer::GetCriticalAreaRatio() const {
    return critical_area_ratio_;
}

IntType Layer::SetCriticalAreaRatio(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Critical area ratio must be nonnegative." << std::endl;
            return 1;
        } else {
            critical_area_ratio_ = value;
            return 0;
        }
    }
}

FloatType Layer::GetClusteringFactor() const {
    return clustering_factor_;
}

IntType Layer::SetClusteringFactor(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Clustering factor must be nonnegative." << std::endl;
            return 1;
        } else {
            clustering_factor_ = value;
            return 0;
        }
    }
}

FloatType Layer::GetLithoPercent() const {
    return litho_percent_;
}

IntType Layer::SetLithoPercent(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value < 0 || value > 1) {
            std::cout << "Error: Litho percent must be between 0 and 1." << std::endl;
            return 1;
        } else {
            litho_percent_ = value;
            return 0;
        }
    }
}

FloatType Layer::GetMaskCost() const {
    return mask_cost_;
}

IntType Layer::SetMaskCost(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Mask cost must be nonnegative." << std::endl;
            return 1;
        } else {
            mask_cost_ = value;
            return 0;
        }
    }
}

FloatType Layer::GetStitchingYield() const {
    return stitching_yield_;
}

IntType Layer::SetStitchingYield(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static layer." << std::endl;
        return 1;
    } else {
        if (value < 0 || value > 1) {
            std::cout << "Error: Stitching yield must be between 0 and 1." << std::endl;
            return 1;
        } else {
            stitching_yield_ = value;
            return 0;
        }
    }
}

BoolType Layer::GetStatic() const {
    return static_;
}

void Layer::SetStatic(BoolType value) {
    static_ = value;
}

BoolType Layer::GetApprox() const {
    return approx_;
}

void Layer::SetApprox(BoolType value) {
    approx_ = value;
}

IntType Layer::SetStatic() {
    if (!LayerFullyDefined()) {
        std::cout << "Error: Attempt to set layer static without defining all parameters. Exiting..." << std::endl;
        std::cout << ToString() << std::endl;
        std::exit(1);
    }
    static_ = true;
    return 0;
}

BoolType Layer::LayerFullyDefined() const {
    // Check if all required parameters are defined (not equal to default values)
    if (name_.empty() ||
        cost_per_mm2_ == DEFAULT_FLOAT_VALUE ||
        transistor_density_ == DEFAULT_FLOAT_VALUE ||
        defect_density_ == DEFAULT_FLOAT_VALUE ||
        critical_area_ratio_ == DEFAULT_FLOAT_VALUE ||
        clustering_factor_ == DEFAULT_FLOAT_VALUE ||
        litho_percent_ == DEFAULT_FLOAT_VALUE ||
        mask_cost_ == DEFAULT_FLOAT_VALUE ||
        stitching_yield_ == DEFAULT_FLOAT_VALUE) {
        return false;
    } else {
        return true;
    }
}

String Layer::ToString() const {
    String return_str = "Layer Name: " + name_;
    return_str += "\n\r\tActive: " + BoolToString(active_);
    return_str += "\n\r\tCost Per mm^2: " + std::to_string(cost_per_mm2_);
    return_str += "\n\r\tTransistor Density: " + std::to_string(transistor_density_);
    return_str += "\n\r\tDefect Density: " + std::to_string(defect_density_);
    return_str += "\n\r\tCritical Area Ratio: " + std::to_string(critical_area_ratio_);
    return_str += "\n\r\tClustering Factor: " + std::to_string(clustering_factor_);
    return_str += "\n\r\tLitho Percent: " + std::to_string(litho_percent_);
    return_str += "\n\r\tMask Cost: " + std::to_string(mask_cost_);
    return_str += "\n\r\tStitching Yield: " + std::to_string(stitching_yield_);
    return_str += "\n\r\tStatic: " + BoolToString(static_);
    return return_str;
}

FloatType Layer::LayerYield(FloatType area) const {
    // Match the Python implementation exactly
    IntType num_stitches = 0;
    FloatType defect_yield = std::pow(1 + (defect_density_ * area * critical_area_ratio_) / clustering_factor_, -1 * clustering_factor_);
    FloatType stitching_yield = std::pow(stitching_yield_, num_stitches);
    FloatType final_layer_yield = stitching_yield * defect_yield;
    return final_layer_yield;
}

FloatType Layer::ReticleUtilization(FloatType area, FloatType reticle_x, FloatType reticle_y) const {
    // Match the Python implementation exactly
    FloatType reticle_area = reticle_x * reticle_y;
    
    // If the area is larger than the reticle area, this requires stitching.
    // To get the reticle utilization, increase the reticle area to the lowest
    // multiple of the reticle area that will fit the stitched chip.
    while (reticle_area < area) {
        reticle_area += reticle_x * reticle_y;
    }
    
    IntType number_chips_in_reticle = static_cast<IntType>(reticle_area / area);
    FloatType unutilized_reticle = reticle_area - number_chips_in_reticle * area;
    FloatType reticle_utilization = (reticle_area - unutilized_reticle) / reticle_area;
    return reticle_utilization;
}

FloatType Layer::ComputeCostPerMm2(FloatType area, FloatType aspect_ratio, const WaferProcess& wafer_process) const {
    // Access parameters that will be used multiple times.
    FloatType wafer_diameter = wafer_process.GetWaferDiameter();
    BoolType grid_fill = wafer_process.GetWaferFillGrid();

    FloatType x_dim = std::sqrt(area * aspect_ratio); //+ wafer_process.dicing_distance
    FloatType y_dim = std::sqrt(area / aspect_ratio); //+ wafer_process.dicing_distance

    // Find effective wafer diameter that is valid for placing dies.
    FloatType usable_wafer_diameter = wafer_diameter - 2 * wafer_process.GetEdgeExclusion();
    // Get the side length of the die assuming it is a square.
    //square_side = math.sqrt(square_area) + wafer_process.dicing_distance

    if (std::sqrt(x_dim * x_dim + y_dim * y_dim) > usable_wafer_diameter / 2) {
        std::cout << "Error: Die size is too large for accurate calculation of fit for wafer. Exiting..." << std::endl;
        //return 1e06; // Return a large number to indicate an error
        std::exit(1);
    }

    if (x_dim == 0 || y_dim == 0) {
        std::cout << "Die size is zero. Exiting..." << std::endl;
        std::exit(1);
    }


    IntType dies_per_wafer = ComputeDiesPerWafer(x_dim, y_dim, usable_wafer_diameter, wafer_process.GetDicingDistance(), grid_fill);

    if (dies_per_wafer == 0) {
        std::cout << "Dies per wafer is zero. Exiting..." << std::endl;
        std::exit(1);
    }

    // Compute the cost per mm^2.
    FloatType used_area = dies_per_wafer * area;
    FloatType circle_area = M_PI * (wafer_diameter / 2) * (wafer_diameter / 2);
    FloatType cost_per_mm2 = cost_per_mm2_ * circle_area / used_area;
    return cost_per_mm2;
}

FloatType Layer::LayerCost(FloatType area, FloatType aspect_ratio, const WaferProcess& wafer_process) const {
    FloatType layer_cost;
    
    // If area is 0, the cost is 0.
    if (area == 0) {
        layer_cost = 0;
    }
    // For valid nonzero area, compute the cost.
    else if (area > 0) {
        // Compute the cost of the layer before considering scaling of lithography costs with reticle fit.
        layer_cost = area * ComputeCostPerMm2(area, aspect_ratio, wafer_process);

        // Get utilization based on reticle fit.
        // Edge case to avoid division by zero.
        FloatType reticle_utilization;
        if (litho_percent_ == 0.0) {
            reticle_utilization = 1.0;
        }
        else if (litho_percent_ > 0.0) {
            reticle_utilization = ReticleUtilization(area, wafer_process.GetReticleX(), wafer_process.GetReticleY());
        }
        // A negative percent does not make sense and should crash the program.
        else {
            std::cout << "Error: Negative litho percent in Layer.layer_cost(). Exiting..." << std::endl;
            std::exit(1);
        }

        // Scale the lithography component of cost by the reticle utilization.
        layer_cost = layer_cost * (1 - litho_percent_) + (layer_cost * litho_percent_) / reticle_utilization;
    }
    // Negative area does not make sense and should crash the program.
    else {
        std::cout << "Error: Negative area in Layer.layer_cost(). Exiting..." << std::endl;
        std::exit(1);
    }

    return layer_cost;
}

IntType Layer::ComputeGridDiesPerWafer(FloatType x_dim, FloatType y_dim, FloatType usable_wafer_diameter, FloatType dicing_distance) const {
   // Run approximate calculation if the flag is set
   if (approx_ == true) {
        FloatType common_term = dicing_distance + std::sqrt(x_dim * y_dim);
        FloatType term1 = usable_wafer_diameter / (4 * std::pow(common_term, 2));
        FloatType term2 = 1 / std::sqrt(2 * std::pow(common_term, 2));
        FloatType approximation_die_per_wafer = std::floor(usable_wafer_diameter * M_PI * (term1 - term2));
        return static_cast<IntType>(approximation_die_per_wafer);
    }
    
    // Input validation
    if (x_dim <= 0 || y_dim <= 0 || usable_wafer_diameter <= 0) {
        return 0;
    }

    // Precompute constants to avoid repeated calculations
    const FloatType r = usable_wafer_diameter * 0.5;
    const FloatType r_squared = r * r;
    const FloatType x_dim_eff = x_dim + dicing_distance;
    const FloatType y_dim_eff = y_dim + dicing_distance;
    const FloatType half_x_dim_eff = x_dim_eff * 0.5;
    const FloatType half_y_dim_eff = y_dim_eff * 0.5;
    const FloatType half_dicing_distance = dicing_distance * 0.5;

    // Calculate the maximum left column height
    const FloatType crossover_column_height = std::sqrt(r_squared - std::pow(half_x_dim_eff - half_dicing_distance, 2)) * 2;
    const IntType max_left_column = static_cast<IntType>(std::ceil(crossover_column_height / half_y_dim_eff)) + 1;

    // Initialize best result
    IntType best_dies_per_wafer = 0;

    // Simple approximation for the specific case
    if (max_left_column > 0 && x_dim >= usable_wafer_diameter * 0.25) {
        const IntType simple_count = static_cast<IntType>(3.14159 * r_squared / (x_dim_eff * y_dim_eff));
        best_dies_per_wafer = simple_count;
    }

#if defined(ENABLE_OPENMP) && !defined(DISABLE_OPENMP) && defined(_OPENMP)
    // Only parallelize if we have enough work to justify the parallelism overhead
    if (max_left_column > 4) {
        // Thread-local best dies counter
        IntType thread_best_dies_per_wafer = best_dies_per_wafer;
        
        #pragma omp parallel
        {
            // Each thread maintains its own best count
            IntType local_best_dies_per_wafer = best_dies_per_wafer;
            
            // Parallelize the outer loop over different left_column_height values
            #pragma omp for schedule(dynamic, 1) nowait
            for (IntType left_column_height = 1; left_column_height < max_left_column; ++left_column_height) {
                // Skip the special case handled above
                if (left_column_height == 1 && x_dim >= usable_wafer_diameter * 0.25) {
                    continue;
                }
                
                IntType dies_per_wafer = 0;
                
                // Calculate first row parameters
                const FloatType row_chord_height = (left_column_height * half_y_dim_eff) - half_dicing_distance;
                if (row_chord_height >= r) {
                    continue; // Skip if row_chord_height is beyond the radius
                }
                
                const FloatType row_chord_height_squared = row_chord_height * row_chord_height;
                const FloatType sqrt_term = std::sqrt(r_squared - row_chord_height_squared);
                const FloatType chord_length = 2.0 * sqrt_term;
                const IntType num_dies_in_row = static_cast<IntType>((chord_length + dicing_distance) / x_dim_eff);
                
                if (num_dies_in_row <= 0) {
                    continue; // Skip if no dies fit in this row
                }
                
                // Add dies in the first block of rows
                dies_per_wafer += num_dies_in_row * left_column_height;
                
                // Handle the far side of the wafer
                const FloatType next_row_chord_height = row_chord_height + y_dim_eff;
                const FloatType half_chord_length = chord_length * 0.5;
                const FloatType end_of_rows = num_dies_in_row * x_dim_eff - half_chord_length;
                const FloatType end_plus_eff = end_of_rows + x_dim_eff;
                const FloatType end_plus_eff_squared = end_plus_eff * end_plus_eff;
                
                // Processing far side dies
                for (IntType i = 0; i < left_column_height; ++i) {
                    const FloatType y = y_dim_eff * i - next_row_chord_height + y_dim_eff;
                    const FloatType y_squared = y * y;
                    
                    // Check if this point is inside the circle
                    if (end_plus_eff_squared + y_squared > r_squared) {
                        continue;
                    }
                    
                    const FloatType y_plus_dim_eff = y + y_dim_eff;
                    const FloatType y_plus_squared = y_plus_dim_eff * y_plus_dim_eff;
                    
                    // Check if the opposite corner is also inside the circle
                    if (end_plus_eff_squared + y_plus_squared <= r_squared) {
                        dies_per_wafer++;
                    }
                }
                
                // Process remaining rows
                FloatType current_row_chord_height = next_row_chord_height;
                const FloatType starting_distance_from_left = (usable_wafer_diameter - chord_length) * 0.5;
                
                while (current_row_chord_height < r) {
                    const FloatType current_chord_height_squared = current_row_chord_height * current_row_chord_height;
                    if (current_chord_height_squared >= r_squared) {
                        break; // Exit if we've reached or exceeded the radius
                    }
                    
                    const FloatType sqrt_term_current = std::sqrt(r_squared - current_chord_height_squared);
                    const FloatType current_chord_length = 2.0 * sqrt_term_current;
                    
                    // Find the first position where a die can fit
                    const FloatType location_of_first_fit = (usable_wafer_diameter - current_chord_length) * 0.5;
                    const FloatType diff = location_of_first_fit - starting_distance_from_left;
                    const FloatType starting_location = std::ceil(diff / x_dim_eff) * x_dim_eff + starting_distance_from_left;
                    const FloatType effective_cord_length = current_chord_length - (starting_location - location_of_first_fit);
                    
                    if (effective_cord_length <= 0) {
                        current_row_chord_height += y_dim_eff;
                        continue; // Skip this row if effective cord length is non-positive
                    }
                    
                    const IntType dies_per_row = static_cast<IntType>(effective_cord_length / x_dim_eff);
                    
                    // Add dies for both top and bottom rows (symmetry)
                    dies_per_wafer += 2 * dies_per_row;
                    
                    current_row_chord_height += y_dim_eff;
                }
                
                // Update thread-local best result if improved
                if (dies_per_wafer > local_best_dies_per_wafer) {
                    local_best_dies_per_wafer = dies_per_wafer;
                }
            }
            
            // Update global best with thread-local best using critical section
            #pragma omp critical
            {
                if (local_best_dies_per_wafer > thread_best_dies_per_wafer) {
                    thread_best_dies_per_wafer = local_best_dies_per_wafer;
                }
            }
        }
        
        // Use the best result found across all threads
        best_dies_per_wafer = thread_best_dies_per_wafer;
    }
    else
#endif
    {
        // Sequential version for small problems or when OpenMP is not available
        // We track dies by count only since we don't use die locations outside this function
        for (IntType left_column_height = 1; left_column_height < max_left_column; ++left_column_height) {
            // Use a faster path for common case (left_column_height = 1)
            if (left_column_height == 1 && x_dim >= usable_wafer_diameter * 0.25) {
                continue;
            }
            
            IntType dies_per_wafer = 0;
            
            // Calculate first row parameters
            const FloatType row_chord_height = (left_column_height * half_y_dim_eff) - half_dicing_distance;
            if (row_chord_height >= r) {
                continue; // Skip if row_chord_height is beyond the radius
            }
            
            const FloatType row_chord_height_squared = row_chord_height * row_chord_height;
            const FloatType sqrt_term = std::sqrt(r_squared - row_chord_height_squared);
            const FloatType chord_length = 2.0 * sqrt_term;
            const IntType num_dies_in_row = static_cast<IntType>((chord_length + dicing_distance) / x_dim_eff);
            
            if (num_dies_in_row <= 0) {
                continue; // Skip if no dies fit in this row
            }
            
            // Add dies in the first block of rows
            dies_per_wafer += num_dies_in_row * left_column_height;
            
            // Handle the far side of the wafer
            const FloatType next_row_chord_height = row_chord_height + y_dim_eff;
            const FloatType half_chord_length = chord_length * 0.5;
            const FloatType end_of_rows = num_dies_in_row * x_dim_eff - half_chord_length;
            const FloatType end_plus_eff = end_of_rows + x_dim_eff;
            const FloatType end_plus_eff_squared = end_plus_eff * end_plus_eff;
            
            // Processing far side dies
            for (IntType i = 0; i < left_column_height; ++i) {
                const FloatType y = y_dim_eff * i - next_row_chord_height + y_dim_eff;
                const FloatType y_squared = y * y;
                
                // Check if this point is inside the circle
                if (end_plus_eff_squared + y_squared > r_squared) {
                    continue;
                }
                
                const FloatType y_plus_dim_eff = y + y_dim_eff;
                const FloatType y_plus_squared = y_plus_dim_eff * y_plus_dim_eff;
                
                // Check if the opposite corner is also inside the circle
                if (end_plus_eff_squared + y_plus_squared <= r_squared) {
                    dies_per_wafer++;
                }
            }
            
            // Process remaining rows
            FloatType current_row_chord_height = next_row_chord_height;
            const FloatType starting_distance_from_left = (usable_wafer_diameter - chord_length) * 0.5;
            
            while (current_row_chord_height < r) {
                const FloatType current_chord_height_squared = current_row_chord_height * current_row_chord_height;
                if (current_chord_height_squared >= r_squared) {
                    break; // Exit if we've reached or exceeded the radius
                }
                
                const FloatType sqrt_term_current = std::sqrt(r_squared - current_chord_height_squared);
                const FloatType current_chord_length = 2.0 * sqrt_term_current;
                
                // Find the first position where a die can fit
                const FloatType location_of_first_fit = (usable_wafer_diameter - current_chord_length) * 0.5;
                const FloatType diff = location_of_first_fit - starting_distance_from_left;
                const FloatType starting_location = std::ceil(diff / x_dim_eff) * x_dim_eff + starting_distance_from_left;
                const FloatType effective_cord_length = current_chord_length - (starting_location - location_of_first_fit);
                
                if (effective_cord_length <= 0) {
                    current_row_chord_height += y_dim_eff;
                    continue; // Skip this row if effective cord length is non-positive
                }
                
                const IntType dies_per_row = static_cast<IntType>(effective_cord_length / x_dim_eff);
                
                // Add dies for both top and bottom rows (symmetry)
                dies_per_wafer += 2 * dies_per_row;
                
                current_row_chord_height += y_dim_eff;
            }
            
            // Update best result if improved
            if (dies_per_wafer > best_dies_per_wafer) {
                best_dies_per_wafer = dies_per_wafer;
            }
        }
    }

    return best_dies_per_wafer;
}

IntType Layer::ComputeNogridDiesPerWafer(FloatType x_dim, FloatType y_dim, FloatType usable_wafer_diameter, FloatType dicing_distance) const {
    // Input validation
    if (x_dim <= 0 || y_dim <= 0 || usable_wafer_diameter <= 0) {
        return 0;
    }

    // Precompute constants to avoid repeated calculations
    const FloatType x_dim_eff = x_dim + dicing_distance;
    const FloatType y_dim_eff = y_dim + dicing_distance;
    const FloatType r = usable_wafer_diameter * 0.5;
    const FloatType r_squared = r * r;
    const FloatType half_dicing_distance = dicing_distance * 0.5;

    // Case 1: Dies are centered on the diameter line of the circle
    IntType num_squares_case_1 = 0;
    std::vector<DieLocation> die_locations_1;
    die_locations_1.reserve(1000); // Pre-reserve space
    
    // Compute the length of a chord that intersects the circle half the square's side length away from the center
    FloatType row_chord_height = y_dim_eff * 0.5;
    
    // Early termination check
    if (row_chord_height - half_dicing_distance >= r) {
        return 0;
    }
    
    FloatType chord_length = std::sqrt(r_squared - std::pow(row_chord_height - half_dicing_distance, 2)) * 2 + dicing_distance;
    
    // Compute the number of squares that fit along the chord length.
    const IntType dies_in_first_row = static_cast<IntType>(std::floor(chord_length / x_dim_eff));
    num_squares_case_1 += dies_in_first_row;
    
    for (IntType j = 0; j < dies_in_first_row; ++j) {
        FloatType x = j * x_dim_eff - chord_length * 0.5;
        FloatType y = -1 * y_dim_eff * 0.5;
        die_locations_1.push_back(DieLocation(x, y));
    }
    
    // Update the row chord height for the next row and start iterating
    row_chord_height += y_dim_eff;
    
    // Store row data for parallel processing
    struct RowData {
        IntType squares_in_row;
        std::vector<DieLocation> locations;
    };
    
    std::vector<RowData> rows_data;
    rows_data.reserve(static_cast<size_t>(r / y_dim_eff)); // Pre-reserve to avoid reallocations
    
    // First collect all row data
    while (row_chord_height < r) {
        // Early termination check for this row
        if (row_chord_height - half_dicing_distance >= r) {
            break;
        }
        
        // Compute the length of a chord that intersects the circle
        FloatType current_chord_length = std::sqrt(r_squared - std::pow(row_chord_height - half_dicing_distance, 2)) * 2 + dicing_distance;
        IntType squares_in_row = static_cast<IntType>(std::floor(current_chord_length / x_dim_eff));
        
        // For the Line Fill case, compute the maximum number of squares that can fit along the chord length
        rows_data.push_back({squares_in_row * 2, {}});
        rows_data.back().locations.reserve(squares_in_row * 2); // Pre-reserve space
        
        for (IntType j = 0; j < squares_in_row; ++j) {
            FloatType x = j * x_dim_eff - current_chord_length * 0.5;
            FloatType y = row_chord_height - y_dim_eff;
            rows_data.back().locations.push_back(DieLocation(x, y));
            rows_data.back().locations.push_back(DieLocation(x, -1 * y - y_dim_eff));
        }
        
        row_chord_height += y_dim_eff;
    }
    
    // Process rows data using multithreading if available
#if defined(ENABLE_OPENMP) && !defined(DISABLE_OPENMP) && defined(_OPENMP)
    // Only parallelize if the problem size is large enough
    if (rows_data.size() > 8) {
        // Define thread-local variables
        IntType total_squares = 0;
        std::vector<DieLocation> total_locations;
        
        #pragma omp parallel
        {
            // Thread-local storage
            std::vector<DieLocation> thread_locations;
            IntType thread_squares = 0;
            
            // Reserve space to avoid reallocations
            thread_locations.reserve(1000);
            
            // Dynamic scheduling for better load balancing
            #pragma omp for schedule(dynamic, 2) nowait
            for (size_t i = 0; i < rows_data.size(); ++i) {
                thread_squares += rows_data[i].squares_in_row;
                thread_locations.insert(thread_locations.end(), 
                                       rows_data[i].locations.begin(), 
                                       rows_data[i].locations.end());
            }
            
            // Combine results with reduced contention
            #pragma omp critical
            {
                total_squares += thread_squares;
                
                // Grow the vector in one operation to minimize reallocation
                if (total_locations.capacity() < total_locations.size() + thread_locations.size()) {
                    total_locations.reserve(total_locations.size() + thread_locations.size());
                }
                
                total_locations.insert(total_locations.end(), 
                                      thread_locations.begin(), 
                                      thread_locations.end());
            }
        }
        
        // Update the global counters
        num_squares_case_1 += total_squares;
        die_locations_1.insert(die_locations_1.end(), 
                              total_locations.begin(), 
                              total_locations.end());
    }
    else 
#endif
    {
        // Sequential version when OpenMP is not available or for small workloads
        for (size_t i = 0; i < rows_data.size(); ++i) {
            num_squares_case_1 += rows_data[i].squares_in_row;
            die_locations_1.insert(die_locations_1.end(), 
                                  rows_data[i].locations.begin(), 
                                  rows_data[i].locations.end());
        }
    }

    // Case 2: Dies are above and below the diameter line of the circle.
    IntType num_squares_case_2 = 0;
    std::vector<DieLocation> die_locations_2;
    die_locations_2.reserve(1000); // Pre-reserve space
    
    // Compute the length of a chord that intersects the circle the square's side length away from the center
    row_chord_height = y_dim_eff;
    
    // Early termination check
    if (row_chord_height - half_dicing_distance >= r) {
        return num_squares_case_1; // Just return case 1 result if we can't fit any rows for case 2
    }
    
    FloatType initial_chord_length = std::sqrt(r_squared - std::pow(row_chord_height - half_dicing_distance, 2)) * 2 + dicing_distance;
    IntType initial_squares = static_cast<IntType>(std::floor(initial_chord_length / x_dim_eff));
    num_squares_case_2 += 2 * initial_squares;

    // Collect row data for case 2
    rows_data.clear();
    rows_data.reserve(static_cast<size_t>(r / y_dim_eff)); // Pre-reserve to avoid reallocations
    
    row_chord_height += y_dim_eff;
    
    while (row_chord_height < r) {
        // Early termination check
        if (row_chord_height - half_dicing_distance >= r) {
            break;
        }
        
        // Compute chord length for this row
        FloatType current_chord_length = std::sqrt(r_squared - std::pow(row_chord_height - half_dicing_distance, 2)) * 2 + dicing_distance;
        IntType squares_in_row = static_cast<IntType>(std::floor(current_chord_length / x_dim_eff));
        
        rows_data.push_back({squares_in_row * 2, {}});
        row_chord_height += y_dim_eff;
    }
    
    // Process case 2 rows using OpenMP if available
#if defined(ENABLE_OPENMP) && !defined(DISABLE_OPENMP) && defined(_OPENMP)
    // Only parallelize if the problem size is large enough
    if (rows_data.size() > 8) {
        // Using parallel reduction for better performance
        IntType parallel_sum = 0;
        
        #pragma omp parallel for reduction(+:parallel_sum) schedule(static)
        for (size_t i = 0; i < rows_data.size(); ++i) {
            parallel_sum += rows_data[i].squares_in_row;
        }
        
        num_squares_case_2 += parallel_sum;
    }
    else 
#endif
    {
        // Sequential version when OpenMP is not available or for small workloads
        for (size_t i = 0; i < rows_data.size(); ++i) {
            num_squares_case_2 += rows_data[i].squares_in_row;
        }
    }

    // Find the maximum of the two cases
    IntType num_squares = std::max(num_squares_case_1, num_squares_case_2);
    
#ifdef __GNUC__
    #pragma GCC pop_options
#endif
    
    return num_squares;
}

IntType Layer::ComputeDiesPerWafer(FloatType x_dim, FloatType y_dim, FloatType usable_wafer_diameter, FloatType dicing_distance, BoolType grid_fill) const {
    // If grid_fill is True, we assume there will be some number of dies flush against the left of the wafer edge.
    //   We need to search through each of these possibilities until another die would fit to the left of this column.
    //   In this case, the 1-die flush case would cover it.
    //   Note that this is still an approximation. The actual number of dies may be slightly higher.
    // If grid_fill is False, we assume that dies are placed in a line along the diameter of the wafer or above and
    //   below the diameter line.

    BoolType simple_equation_flag = false;
    IntType num_squares;

    if (simple_equation_flag) {
        num_squares = static_cast<IntType>(usable_wafer_diameter * M_PI * 
                      ((usable_wafer_diameter / (4 * (y_dim + dicing_distance) * (x_dim + dicing_distance))) - 
                       (1 / std::sqrt(2 * (y_dim + dicing_distance) * (x_dim + dicing_distance)))));
    } else {
        if (grid_fill) {
            num_squares = ComputeGridDiesPerWafer(x_dim, y_dim, usable_wafer_diameter, dicing_distance);
        } else {
            num_squares = ComputeNogridDiesPerWafer(x_dim, y_dim, usable_wafer_diameter, dicing_distance);
        }
    }

    return num_squares;
}

std::ostream& operator<<(std::ostream& os, const Layer& layer) {
    os << layer.ToString();
    return os;
}

} // namespace design 