#ifndef WAFER_PROCESS_HPP
#define WAFER_PROCESS_HPP

#include "DesignCommon.hpp"
#include <string>
#include <iostream>

namespace design {

/**
 * @class WaferProcess
 * @brief Represents a wafer manufacturing process with its parameters
 * 
 * The class has the following attributes:
 *   name: The name of the wafer process.
 *   wafer_diameter: The diameter of the wafer in mm.
 *   edge_exclusion: The edge exclusion of the wafer in mm.
 *   wafer_process_yield: The yield of the wafer process. Value should be between 0 and 1.
 *   dicing_distance: The dicing distance in mm.
 *   reticle_x: Reticle dimension in the x dimension in mm.
 *   reticle_y: Reticle dimension in the y dimension in mm.
 *   wafer_fill_grid: Whether the wafer is filled in a grid pattern
 *   nre_front_end_cost_per_mm2_memory: NRE design cost per mm^2 for memory front end
 *   nre_back_end_cost_per_mm2_memory: NRE design cost per mm^2 for memory back end
 *   nre_front_end_cost_per_mm2_logic: NRE design cost per mm^2 for logic front end
 *   nre_back_end_cost_per_mm2_logic: NRE design cost per mm^2 for logic back end
 *   nre_front_end_cost_per_mm2_analog: NRE design cost per mm^2 for analog front end
 *   nre_back_end_cost_per_mm2_analog: NRE design cost per mm^2 for analog back end
 *   static: Whether the wafer process definition is finalized
 */
class WaferProcess {
public:
    /**
     * @brief Constructor for WaferProcess
     */
    WaferProcess(
        const String& name = DEFAULT_STRING_VALUE,
        FloatType wafer_diameter = DEFAULT_FLOAT_VALUE,
        FloatType edge_exclusion = DEFAULT_FLOAT_VALUE,
        FloatType wafer_process_yield = DEFAULT_FLOAT_VALUE,
        FloatType dicing_distance = DEFAULT_FLOAT_VALUE,
        FloatType reticle_x = DEFAULT_FLOAT_VALUE,
        FloatType reticle_y = DEFAULT_FLOAT_VALUE,
        const String& wafer_fill_grid = DEFAULT_STRING_VALUE,
        FloatType nre_front_end_cost_per_mm2_memory = DEFAULT_FLOAT_VALUE,
        FloatType nre_back_end_cost_per_mm2_memory = DEFAULT_FLOAT_VALUE,
        FloatType nre_front_end_cost_per_mm2_logic = DEFAULT_FLOAT_VALUE,
        FloatType nre_back_end_cost_per_mm2_logic = DEFAULT_FLOAT_VALUE,
        FloatType nre_front_end_cost_per_mm2_analog = DEFAULT_FLOAT_VALUE,
        FloatType nre_back_end_cost_per_mm2_analog = DEFAULT_FLOAT_VALUE,
        BoolType static_value = true);

    // Getters and setters
    String GetName() const;
    IntType SetName(const String& value);

    FloatType GetWaferDiameter() const;
    IntType SetWaferDiameter(FloatType value);

    FloatType GetEdgeExclusion() const;
    IntType SetEdgeExclusion(FloatType value);

    FloatType GetWaferProcessYield() const;
    IntType SetWaferProcessYield(FloatType value);

    FloatType GetDicingDistance() const;
    IntType SetDicingDistance(FloatType value);

    FloatType GetReticleX() const;
    IntType SetReticleX(FloatType value);

    FloatType GetReticleY() const;
    IntType SetReticleY(FloatType value);

    BoolType GetWaferFillGrid() const;
    IntType SetWaferFillGrid(const String& value);

    FloatType GetNreFrontEndCostPerMm2Memory() const;
    IntType SetNreFrontEndCostPerMm2Memory(FloatType value);

    FloatType GetNreBackEndCostPerMm2Memory() const;
    IntType SetNreBackEndCostPerMm2Memory(FloatType value);

    FloatType GetNreFrontEndCostPerMm2Logic() const;
    IntType SetNreFrontEndCostPerMm2Logic(FloatType value);

    FloatType GetNreBackEndCostPerMm2Logic() const;
    IntType SetNreBackEndCostPerMm2Logic(FloatType value);

    FloatType GetNreFrontEndCostPerMm2Analog() const;
    IntType SetNreFrontEndCostPerMm2Analog(FloatType value);

    FloatType GetNreBackEndCostPerMm2Analog() const;
    IntType SetNreBackEndCostPerMm2Analog(FloatType value);

    BoolType GetStatic() const;
    IntType SetStatic(BoolType value);

    /**
     * @brief Set the wafer process as static (unchangeable)
     * 
     * @return 0 on success, 1 on failure
     */
    IntType SetStatic();

    /**
     * @brief Check if the wafer process is fully defined
     * 
     * @return true if fully defined, false otherwise
     */
    BoolType WaferFullyDefined() const;

    /**
     * @brief Convert wafer process to string representation
     * 
     * @return String representation of wafer process
     */
    String ToString() const;

    // Operator overload for stream output
    friend std::ostream& operator<<(std::ostream& os, const WaferProcess& wafer_process);

private:
    // Private member variables (equivalent to Python's __variables)
    String name_;
    FloatType wafer_diameter_;
    FloatType edge_exclusion_;
    FloatType wafer_process_yield_;
    FloatType dicing_distance_;
    FloatType reticle_x_;
    FloatType reticle_y_;
    BoolType wafer_fill_grid_;
    FloatType nre_front_end_cost_per_mm2_memory_;
    FloatType nre_back_end_cost_per_mm2_memory_;
    FloatType nre_front_end_cost_per_mm2_logic_;
    FloatType nre_back_end_cost_per_mm2_logic_;
    FloatType nre_front_end_cost_per_mm2_analog_;
    FloatType nre_back_end_cost_per_mm2_analog_;
    BoolType static_;
};

} // namespace design

#endif // WAFER_PROCESS_HPP 