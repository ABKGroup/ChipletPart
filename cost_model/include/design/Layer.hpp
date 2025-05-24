#ifndef LAYER_HPP
#define LAYER_HPP

#include "DesignCommon.hpp"
#include <iostream>
#include <vector>
#include <cmath>

namespace design {

// Forward declaration of WaferProcess class to avoid circular dependency
class WaferProcess;

/**
 * @class Layer
 * @brief Represents a manufacturing layer with its parameters
 * 
 * The class has the following attributes:
 *   name: The name of the layer.
 *   active: Whether the layer is active or not.
 *   cost_per_mm2: The cost per mm^2 of the layer.
 *   transistor_density: The transistor density of the layer in millions per mm^2.
 *   defect_density: The defect density of the layer.
 *   critical_area_ratio: The critical area ratio of the layer.
 *   clustering_factor: The clustering factor of the layer.
 *   litho_percent: The litho percent of the layer.
 *   mask_cost: The mask cost of the layer.
 *   stitching_yield: The stitching yield of the layer.
 *   static: A boolean set true when the layer is defined to prevent further changes.
 */
class Layer {
public:
    /**
     * @brief Constructor for Layer
     * 
     * @param name The name of the layer
     * @param active Whether the layer is active or not
     * @param cost_per_mm2 The cost per mm^2 of the layer
     * @param transistor_density The transistor density in millions per mm^2
     * @param defect_density The defect density of the layer
     * @param critical_area_ratio The critical area ratio of the layer
     * @param clustering_factor The clustering factor of the layer
     * @param litho_percent The litho percent of the layer
     * @param mask_cost The mask cost of the layer
     * @param stitching_yield The stitching yield of the layer
     * @param static_value Whether the layer is static (unchangeable)
     */
    Layer(
        const String& name = DEFAULT_STRING_VALUE,
        const String& active = DEFAULT_STRING_VALUE,
        FloatType cost_per_mm2 = DEFAULT_FLOAT_VALUE,
        FloatType transistor_density = DEFAULT_FLOAT_VALUE,
        FloatType defect_density = DEFAULT_FLOAT_VALUE,
        FloatType critical_area_ratio = DEFAULT_FLOAT_VALUE,
        FloatType clustering_factor = DEFAULT_FLOAT_VALUE,
        FloatType litho_percent = DEFAULT_FLOAT_VALUE,
        FloatType mask_cost = DEFAULT_FLOAT_VALUE,
        FloatType stitching_yield = DEFAULT_FLOAT_VALUE,
        BoolType static_value = true);

    // Getters and setters for all properties
    String GetName() const;
    IntType SetName(const String& value);

    BoolType GetActive() const;
    IntType SetActive(const String& value);

    FloatType GetCostPerMm2() const;
    IntType SetCostPerMm2(FloatType value);

    FloatType GetTransistorDensity() const;
    IntType SetTransistorDensity(FloatType value);

    FloatType GetDefectDensity() const;
    IntType SetDefectDensity(FloatType value);

    FloatType GetCriticalAreaRatio() const;
    IntType SetCriticalAreaRatio(FloatType value);

    FloatType GetClusteringFactor() const;
    IntType SetClusteringFactor(FloatType value);

    FloatType GetLithoPercent() const;
    IntType SetLithoPercent(FloatType value);

    FloatType GetMaskCost() const;
    IntType SetMaskCost(FloatType value);

    FloatType GetStitchingYield() const;
    IntType SetStitchingYield(FloatType value);

    BoolType GetStatic() const;
    void SetStatic(BoolType value);

    BoolType GetApprox() const;
    void SetApprox(BoolType value);

    /**
     * @brief Get the number of gates per mm^2
     * 
     * @return Number of gates per mm^2
     */
    FloatType GetGatesPerMm2() const;

    /**
     * @brief Check if all layer parameters are defined
     * 
     * @return true if all parameters are defined, false otherwise
     */
    BoolType LayerFullyDefined() const;

    /**
     * @brief Set the layer as static (unchangeable)
     * 
     * @return 0 on success, 1 on failure
     */
    IntType SetStatic();

    /**
     * @brief Convert layer to string representation
     * 
     * @return String representation of the layer
     */
    String ToString() const;
    
    /**
     * @brief Calculate layer yield based on area
     * 
     * @param area Area in mm^2
     * @return Layer yield
     */
    FloatType LayerYield(FloatType area) const;
    
    /**
     * @brief Calculate reticle utilization
     * 
     * @param area Area in mm^2
     * @param reticle_x Reticle x dimension in mm
     * @param reticle_y Reticle y dimension in mm
     * @return Reticle utilization
     */
    FloatType ReticleUtilization(FloatType area, FloatType reticle_x, FloatType reticle_y) const;
    
    /**
     * @brief Calculate layer cost
     * 
     * @param area Area in mm^2
     * @param aspect_ratio Aspect ratio of the chip
     * @param wafer_process Wafer process reference
     * @return Layer cost
     */
    FloatType LayerCost(FloatType area, FloatType aspect_ratio, const WaferProcess& wafer_process) const;
    
    /**
     * @brief Compute the cost per mm^2 considering various factors
     * 
     * @param area Area in mm^2
     * @param aspect_ratio Aspect ratio of the chip
     * @param wafer_process Wafer process reference
     * @return Cost per mm^2
     */
    FloatType ComputeCostPerMm2(FloatType area, FloatType aspect_ratio, const WaferProcess& wafer_process) const;
    
    /**
     * @brief Compute the number of dies per wafer in a grid pattern
     * 
     * @param x_dim X dimension in mm
     * @param y_dim Y dimension in mm
     * @param usable_wafer_diameter Usable wafer diameter in mm
     * @param dicing_distance Dicing distance in mm
     * @return Number of dies per wafer
     */
    IntType ComputeGridDiesPerWafer(FloatType x_dim, FloatType y_dim, FloatType usable_wafer_diameter, FloatType dicing_distance) const;
    
    /**
     * @brief Compute the number of dies per wafer without using a grid layout
     * 
     * @param x_dim X dimension in mm
     * @param y_dim Y dimension in mm
     * @param usable_wafer_diameter Usable wafer diameter in mm
     * @param dicing_distance Dicing distance in mm
     * @return Number of dies per wafer
     */
    IntType ComputeNogridDiesPerWafer(FloatType x_dim, FloatType y_dim, FloatType usable_wafer_diameter, FloatType dicing_distance) const;
    
    /**
     * @brief Compute the number of dies per wafer, choosing between grid and no-grid calculations
     * 
     * @param x_dim X dimension in mm
     * @param y_dim Y dimension in mm
     * @param usable_wafer_diameter Usable wafer diameter in mm
     * @param dicing_distance Dicing distance in mm
     * @param grid_fill Whether to use grid layout
     * @return Number of dies per wafer
     */
    IntType ComputeDiesPerWafer(FloatType x_dim, FloatType y_dim, FloatType usable_wafer_diameter, FloatType dicing_distance, BoolType grid_fill) const;
    
    // Operator overload for stream output
    friend std::ostream& operator<<(std::ostream& os, const Layer& layer);

private:
    // Private member variables (equivalent to Python's __variables)
    String name_;
    BoolType active_;
    FloatType cost_per_mm2_;
    FloatType transistor_density_;
    FloatType defect_density_;
    FloatType critical_area_ratio_;
    FloatType clustering_factor_;
    FloatType litho_percent_;
    FloatType mask_cost_;
    FloatType stitching_yield_;
    BoolType static_;
    BoolType approx_ = false; // Default value for approx_
    
    /**
     * @brief Helper structure to represent a die location on a wafer
     */
    struct DieLocation {
        FloatType x;
        FloatType y;
        
        DieLocation(FloatType x_pos, FloatType y_pos) : x(x_pos), y(y_pos) {}
    };
};

} // namespace design

#endif // LAYER_HPP 