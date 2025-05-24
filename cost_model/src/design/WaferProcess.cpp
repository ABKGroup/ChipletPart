#include "design/WaferProcess.hpp"
#include <iostream>
#include <cstdlib>
#include <algorithm> // For std::transform

namespace design {

WaferProcess::WaferProcess(
    const String& name,
    FloatType wafer_diameter,
    FloatType edge_exclusion,
    FloatType wafer_process_yield,
    FloatType dicing_distance,
    FloatType reticle_x,
    FloatType reticle_y,
    const String& wafer_fill_grid,
    FloatType nre_front_end_cost_per_mm2_memory,
    FloatType nre_back_end_cost_per_mm2_memory,
    FloatType nre_front_end_cost_per_mm2_logic,
    FloatType nre_back_end_cost_per_mm2_logic,
    FloatType nre_front_end_cost_per_mm2_analog,
    FloatType nre_back_end_cost_per_mm2_analog,
    BoolType static_value)
{
    // In Python, first initialize static to False
    static_ = false;
    
    // Directly set properties without validation, matching Python implementation
    name_ = name;
    wafer_diameter_ = wafer_diameter;
    edge_exclusion_ = edge_exclusion;
    wafer_process_yield_ = wafer_process_yield;
    dicing_distance_ = dicing_distance;
    reticle_x_ = reticle_x;
    reticle_y_ = reticle_y;
    
    // For wafer_fill_grid, convert string to bool (similar to Python logic)
    if (!wafer_fill_grid.empty()) {
        wafer_fill_grid_ = StringToBool(wafer_fill_grid);
    }
    
    nre_front_end_cost_per_mm2_memory_ = nre_front_end_cost_per_mm2_memory;
    nre_back_end_cost_per_mm2_memory_ = nre_back_end_cost_per_mm2_memory;
    nre_front_end_cost_per_mm2_logic_ = nre_front_end_cost_per_mm2_logic;
    nre_back_end_cost_per_mm2_logic_ = nre_back_end_cost_per_mm2_logic;
    nre_front_end_cost_per_mm2_analog_ = nre_front_end_cost_per_mm2_analog;
    nre_back_end_cost_per_mm2_analog_ = nre_back_end_cost_per_mm2_analog;
    
    // Set static last
    static_ = static_value;
    
    // Check if fully defined and update static if needed
    if (!WaferFullyDefined()) {
        std::cout << "Warning: Wafer Process not fully defined, setting to non-static." << std::endl;
        static_ = false;
        //std::cout << ToString() << std::endl;
    }
}

String WaferProcess::GetName() const {
    return name_;
}

IntType WaferProcess::SetName(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Wafer process name must be a string." << std::endl;
            return 1;
        } else {
            name_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetWaferDiameter() const {
    return wafer_diameter_;
}

IntType WaferProcess::SetWaferDiameter(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Wafer diameter must be nonnegative." << std::endl;
            return 1;
        } else {
            wafer_diameter_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetEdgeExclusion() const {
    return edge_exclusion_;
}

IntType WaferProcess::SetEdgeExclusion(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Edge exclusion must be nonnegative." << std::endl;
            return 1;
        } else if (value > wafer_diameter_ / 2) {
            std::cout << "Error: Edge exclusion must be less than half the wafer diameter." << std::endl;
            return 1;
        } else {
            edge_exclusion_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetWaferProcessYield() const {
    return wafer_process_yield_;
}

IntType WaferProcess::SetWaferProcessYield(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0.0 || value > 1.0) {
            std::cout << "Error: Wafer process yield must be between 0 and 1." << std::endl;
            return 1;
        } else {
            wafer_process_yield_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetDicingDistance() const {
    return dicing_distance_;
}

IntType WaferProcess::SetDicingDistance(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Dicing distance must be nonnegative." << std::endl;
            return 1;
        } else if (value > wafer_diameter_ / 2) {
            std::cout << "Error: Dicing distance must be less than half the wafer diameter." << std::endl;
            return 1;
        } else {
            dicing_distance_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetReticleX() const {
    return reticle_x_;
}

IntType WaferProcess::SetReticleX(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Reticle x dimension must be nonnegative." << std::endl;
            return 1;
        } else if (value > wafer_diameter_ / 2) {
            std::cout << "Error: Reticle x dimension must be less than half the wafer diameter." << std::endl;
            return 1;
        } else {
            reticle_x_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetReticleY() const {
    return reticle_y_;
}

IntType WaferProcess::SetReticleY(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Reticle y dimension must be nonnegative." << std::endl;
            return 1;
        } else if (value > wafer_diameter_ / 2) {
            std::cout << "Error: Reticle y dimension must be less than half the wafer diameter." << std::endl;
            return 1;
        } else {
            reticle_y_ = value;
            return 0;
        }
    }
}

BoolType WaferProcess::GetWaferFillGrid() const {
    return wafer_fill_grid_;
}

IntType WaferProcess::SetWaferFillGrid(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        // Since C++ is statically typed, we're already guaranteed this is a string
        // So we don't need to print the type error message that Python would print
        
        // Use utility function from DesignCommon.hpp to convert the string to bool
        wafer_fill_grid_ = StringToBool(value);
        return 0;
    }
}

FloatType WaferProcess::GetNreFrontEndCostPerMm2Memory() const {
    return nre_front_end_cost_per_mm2_memory_;
}

IntType WaferProcess::SetNreFrontEndCostPerMm2Memory(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: NRE front end cost per mm2 memory must be nonnegative." << std::endl;
            return 1;
        } else {
            nre_front_end_cost_per_mm2_memory_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetNreBackEndCostPerMm2Memory() const {
    return nre_back_end_cost_per_mm2_memory_;
}

IntType WaferProcess::SetNreBackEndCostPerMm2Memory(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: NRE back end cost per mm2 memory must be nonnegative." << std::endl;
            return 1;
        } else {
            nre_back_end_cost_per_mm2_memory_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetNreFrontEndCostPerMm2Logic() const {
    return nre_front_end_cost_per_mm2_logic_;
}

IntType WaferProcess::SetNreFrontEndCostPerMm2Logic(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: NRE front end cost per mm2 logic must be nonnegative." << std::endl;
            return 1;
        } else {
            nre_front_end_cost_per_mm2_logic_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetNreBackEndCostPerMm2Logic() const {
    return nre_back_end_cost_per_mm2_logic_;
}

IntType WaferProcess::SetNreBackEndCostPerMm2Logic(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: NRE back end cost per mm2 logic must be nonnegative." << std::endl;
            return 1;
        } else {
            nre_back_end_cost_per_mm2_logic_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetNreFrontEndCostPerMm2Analog() const {
    return nre_front_end_cost_per_mm2_analog_;
}

IntType WaferProcess::SetNreFrontEndCostPerMm2Analog(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: NRE front end cost per mm2 analog must be nonnegative." << std::endl;
            return 1;
        } else {
            nre_front_end_cost_per_mm2_analog_ = value;
            return 0;
        }
    }
}

FloatType WaferProcess::GetNreBackEndCostPerMm2Analog() const {
    return nre_back_end_cost_per_mm2_analog_;
}

IntType WaferProcess::SetNreBackEndCostPerMm2Analog(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static wafer process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: NRE back end cost per mm2 analog must be nonnegative." << std::endl;
            return 1;
        } else {
            nre_back_end_cost_per_mm2_analog_ = value;
            return 0;
        }
    }
}

BoolType WaferProcess::GetStatic() const {
    return static_;
}

IntType WaferProcess::SetStatic(BoolType value) {
    static_ = value;
    return 0;
}

IntType WaferProcess::SetStatic() {
    if (!WaferFullyDefined()) {
        std::cout << "Error: Attempt to set wafer process static without defining all parameters. Exiting..." << std::endl;
        std::cout << ToString() << std::endl;
        std::exit(1);
    }
    static_ = true;
    return 0;
}

BoolType WaferProcess::WaferFullyDefined() const {
    // In Python, this function just checks for None (uninitialized) values
    // In C++, we'll check for default values, which is the closest equivalent
    if (name_.empty() ||
        wafer_diameter_ == DEFAULT_FLOAT_VALUE ||
        edge_exclusion_ == DEFAULT_FLOAT_VALUE ||
        wafer_process_yield_ == DEFAULT_FLOAT_VALUE ||
        dicing_distance_ == DEFAULT_FLOAT_VALUE ||
        reticle_x_ == DEFAULT_FLOAT_VALUE ||
        reticle_y_ == DEFAULT_FLOAT_VALUE ||
        nre_front_end_cost_per_mm2_memory_ == DEFAULT_FLOAT_VALUE ||
        nre_back_end_cost_per_mm2_memory_ == DEFAULT_FLOAT_VALUE ||
        nre_front_end_cost_per_mm2_logic_ == DEFAULT_FLOAT_VALUE ||
        nre_back_end_cost_per_mm2_logic_ == DEFAULT_FLOAT_VALUE ||
        nre_front_end_cost_per_mm2_analog_ == DEFAULT_FLOAT_VALUE ||
        nre_back_end_cost_per_mm2_analog_ == DEFAULT_FLOAT_VALUE) {
        return false;
    }
    return true;
}

String WaferProcess::ToString() const {
    String return_str = "Wafer Process: " + name_;
    return_str += "\n\r\tWafer Diameter: " + std::to_string(wafer_diameter_);
    return_str += "\n\r\tEdge Exclusion: " + std::to_string(edge_exclusion_);
    return_str += "\n\r\tWafer Process Yield: " + std::to_string(wafer_process_yield_);
    return_str += "\n\r\tDicing Distance: " + std::to_string(dicing_distance_);
    return_str += "\n\r\tReticle X: " + std::to_string(reticle_x_);
    return_str += "\n\r\tReticle Y: " + std::to_string(reticle_y_);
    return_str += "\n\r\tWafer Fill Grid: " + BoolToString(wafer_fill_grid_);
    return_str += "\n\r\tNRE Front End Cost per mm2 Memory: " + std::to_string(nre_front_end_cost_per_mm2_memory_);
    return_str += "\n\r\tNRE Back End Cost per mm2 Memory: " + std::to_string(nre_back_end_cost_per_mm2_memory_);
    return_str += "\n\r\tNRE Front End Cost per mm2 Logic: " + std::to_string(nre_front_end_cost_per_mm2_logic_);
    return_str += "\n\r\tNRE Back End Cost per mm2 Logic: " + std::to_string(nre_back_end_cost_per_mm2_logic_);
    return_str += "\n\r\tNRE Front End Cost per mm2 Analog: " + std::to_string(nre_front_end_cost_per_mm2_analog_);
    return_str += "\n\r\tNRE Back End Cost per mm2 Analog: " + std::to_string(nre_back_end_cost_per_mm2_analog_);
    return_str += "\n\r\tStatic: " + BoolToString(static_);
    return return_str;
}

std::ostream& operator<<(std::ostream& os, const WaferProcess& wafer_process) {
    os << wafer_process.ToString();
    return os;
}

} // namespace design 