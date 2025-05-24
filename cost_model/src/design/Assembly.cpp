#include "design/Assembly.hpp"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <cmath>

namespace design {

Assembly::Assembly(
    const String& name,
    FloatType materials_cost_per_mm2,
    FloatType bb_cost_per_second,
    FloatType picknplace_machine_cost,
    FloatType picknplace_machine_lifetime,
    FloatType picknplace_machine_uptime,
    FloatType picknplace_technician_yearly_cost,
    FloatType picknplace_time,
    FloatType picknplace_group,
    FloatType bonding_machine_cost,
    FloatType bonding_machine_lifetime,
    FloatType bonding_machine_uptime,
    FloatType bonding_technician_yearly_cost,
    FloatType bonding_time,
    FloatType bonding_group,
    FloatType die_separation,
    FloatType edge_exclusion,
    FloatType max_pad_current_density,
    FloatType bonding_pitch,
    FloatType alignment_yield,
    FloatType bonding_yield,
    FloatType dielectric_bond_defect_density,
    BoolType static_value)
    : static_(false)  // Initialize static_ to false first
{
    // Initialize all properties
    name_ = name;
    materials_cost_per_mm2_ = materials_cost_per_mm2;
    bb_cost_per_second_ = bb_cost_per_second;
    picknplace_machine_cost_ = picknplace_machine_cost;
    picknplace_machine_lifetime_ = picknplace_machine_lifetime;
    picknplace_machine_uptime_ = picknplace_machine_uptime;
    picknplace_technician_yearly_cost_ = picknplace_technician_yearly_cost;
    picknplace_time_ = picknplace_time;
    picknplace_group_ = picknplace_group;
    bonding_machine_cost_ = bonding_machine_cost;
    bonding_machine_lifetime_ = bonding_machine_lifetime;
    bonding_machine_uptime_ = bonding_machine_uptime;
    bonding_technician_yearly_cost_ = bonding_technician_yearly_cost;
    bonding_time_ = bonding_time;
    bonding_group_ = bonding_group;
    die_separation_ = die_separation;
    edge_exclusion_ = edge_exclusion;
    max_pad_current_density_ = max_pad_current_density;
    bonding_pitch_ = bonding_pitch;
    alignment_yield_ = alignment_yield;
    bonding_yield_ = bonding_yield;
    dielectric_bond_defect_density_ = dielectric_bond_defect_density;
    
    // Initialize derived properties to DEFAULT_FLOAT_VALUE (equivalent to None in Python)
    picknplace_cost_per_second_ = DEFAULT_FLOAT_VALUE;
    bonding_cost_per_second_ = DEFAULT_FLOAT_VALUE;
    
    // Set static value last
    static_ = static_value;
    
    // If not fully defined, set to non-static
    if (!AssemblyFullyDefined()) {
        std::cout << "Warning: Assembly not fully defined. Setting non-static." << std::endl;
        static_ = false;
        //std::cout << ToString() << std::endl;
    }
    else {
        // Only compute derived properties if the assembly is fully defined
        picknplace_cost_per_second_ = ComputePicknplaceCostPerSecond();
        bonding_cost_per_second_ = ComputeBondingCostPerSecond();
    }
}

String Assembly::GetName() const {
    return name_;
}

IntType Assembly::SetName(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Assembly process name must be a string." << std::endl;
            return 1;
        } else {
            name_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetMaterialsCostPerMm2() const {
    return materials_cost_per_mm2_;
}

IntType Assembly::SetMaterialsCostPerMm2(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Materials cost per mm^2 must be nonnegative." << std::endl;
            return 1;
        } else {
            materials_cost_per_mm2_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBbCostPerSecond() const {
    return bb_cost_per_second_;
}

IntType Assembly::SetBbCostPerSecond(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_cost_per_second_ = value;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: Pick and place cost per second must be nonnegative." << std::endl;
            return 1;
        } else {
            bb_cost_per_second_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetPicknplaceMachineCost() const {
    return picknplace_machine_cost_;
}

IntType Assembly::SetPicknplaceMachineCost(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Pick and place machine cost must be nonnegative." << std::endl;
            return 1;
        } else {
            picknplace_machine_cost_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetPicknplaceMachineLifetime() const {
    return picknplace_machine_lifetime_;
}

IntType Assembly::SetPicknplaceMachineLifetime(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Pick and place machine lifetime must be nonnegative." << std::endl;
            return 1;
        } else {
            picknplace_machine_lifetime_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetPicknplaceMachineUptime() const {
    return picknplace_machine_uptime_;
}

IntType Assembly::SetPicknplaceMachineUptime(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0 || value > 1) {
            std::cout << "Error: Pick and place machine uptime must be between 0 and 1." << std::endl;
            return 1;
        } else {
            picknplace_machine_uptime_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetPicknplaceTechnicianYearlyCost() const {
    return picknplace_technician_yearly_cost_;
}

IntType Assembly::SetPicknplaceTechnicianYearlyCost(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Pick and place technician yearly cost must be nonnegative." << std::endl;
            return 1;
        } else {
            picknplace_technician_yearly_cost_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetPicknplaceTime() const {
    return picknplace_time_;
}

IntType Assembly::SetPicknplaceTime(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Pick and place time must be nonnegative." << std::endl;
            return 1;
        } else {
            picknplace_time_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetPicknplaceGroup() const {
    return picknplace_group_;
}

IntType Assembly::SetPicknplaceGroup(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Pick and place group must be nonnegative." << std::endl;
            return 1;
        } else {
            picknplace_group_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBondingMachineCost() const {
    return bonding_machine_cost_;
}

IntType Assembly::SetBondingMachineCost(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Bonding machine cost must be nonnegative." << std::endl;
            return 1;
        } else {
            bonding_machine_cost_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBondingMachineLifetime() const {
    return bonding_machine_lifetime_;
}

IntType Assembly::SetBondingMachineLifetime(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Bonding machine lifetime must be nonnegative." << std::endl;
            return 1;
        } else {
            bonding_machine_lifetime_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBondingMachineUptime() const {
    return bonding_machine_uptime_;
}

IntType Assembly::SetBondingMachineUptime(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0 || value > 1) {
            std::cout << "Error: Bonding machine uptime must be between 0 and 1." << std::endl;
            return 1;
        } else {
            bonding_machine_uptime_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBondingTechnicianYearlyCost() const {
    return bonding_technician_yearly_cost_;
}

IntType Assembly::SetBondingTechnicianYearlyCost(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Bonding technician yearly cost must be nonnegative." << std::endl;
            return 1;
        } else {
            bonding_technician_yearly_cost_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBondingTime() const {
    return bonding_time_;
}

IntType Assembly::SetBondingTime(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Bonding time must be nonnegative." << std::endl;
            return 1;
        } else {
            bonding_time_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBondingGroup() const {
    return bonding_group_;
}

IntType Assembly::SetBondingGroup(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Bonding group must be nonnegative." << std::endl;
            return 1;
        } else {
            bonding_group_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetDieSeparation() const {
    return die_separation_;
}

IntType Assembly::SetDieSeparation(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Die separation must be nonnegative." << std::endl;
            return 1;
        } else {
            die_separation_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetEdgeExclusion() const {
    return edge_exclusion_;
}

IntType Assembly::SetEdgeExclusion(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Edge exclusion must be nonnegative." << std::endl;
            return 1;
        } else {
            edge_exclusion_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetMaxPadCurrentDensity() const {
    return max_pad_current_density_;
}

IntType Assembly::SetMaxPadCurrentDensity(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        // In Python, there's no check for negative values, only type checking
        // Since C++ ensures type at compile time, we just directly set the value
        max_pad_current_density_ = value;
        return 0;
    }
}

FloatType Assembly::GetBondingPitch() const {
    return bonding_pitch_;
}

IntType Assembly::SetBondingPitch(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Bonding pitch must be nonnegative." << std::endl;
            return 1;
        } else {
            bonding_pitch_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetAlignmentYield() const {
    return alignment_yield_;
}

IntType Assembly::SetAlignmentYield(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0 || value > 1) {
            std::cout << "Error: Alignment yield must be between 0 and 1." << std::endl;
            return 1;
        } else {
            alignment_yield_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBondingYield() const {
    return bonding_yield_;
}

IntType Assembly::SetBondingYield(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0 || value > 1) {
            std::cout << "Error: Bonding yield must be between 0 and 1." << std::endl;
            return 1;
        } else {
            bonding_yield_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetDielectricBondDefectDensity() const {
    return dielectric_bond_defect_density_;
}

IntType Assembly::SetDielectricBondDefectDensity(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Dielectric bond defect density must be nonnegative." << std::endl;
            return 1;
        } else {
            dielectric_bond_defect_density_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetPicknplaceCostPerSecond() const {
    return picknplace_cost_per_second_;
}

IntType Assembly::SetPicknplaceCostPerSecond(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            // This handles the case for None/null in Python
            picknplace_cost_per_second_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: Pick and place cost per second must be nonnegative." << std::endl;
            return 1;
        } else {
            picknplace_cost_per_second_ = value;
            return 0;
        }
    }
}

FloatType Assembly::GetBondingCostPerSecond() const {
    return bonding_cost_per_second_;
}

IntType Assembly::SetBondingCostPerSecond(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static assembly process." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            // This handles the case for None/null in Python
            bonding_cost_per_second_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: Bonding cost per second must be nonnegative." << std::endl;
            return 1;
        } else {
            bonding_cost_per_second_ = value;
            return 0;
        }
    }
}

BoolType Assembly::GetStatic() const {
    return static_;
}

void Assembly::SetStatic(BoolType value) {
    static_ = value;
}

IntType Assembly::SetStatic() {
    if (!AssemblyFullyDefined()) {
        std::cout << "Error: Attempt to set assembly process static without defining all parameters. Exiting..." << std::endl;
        //std::cout << ToString() << std::endl;
        std::exit(1);
    }
    static_ = true;
    return 0;
}

BoolType Assembly::AssemblyFullyDefined() const {
    // Match the Python assembly_fully_defined exactly
    if (name_.empty() ||
        materials_cost_per_mm2_ == DEFAULT_FLOAT_VALUE ||
        picknplace_machine_cost_ == DEFAULT_FLOAT_VALUE ||
        picknplace_machine_lifetime_ == DEFAULT_FLOAT_VALUE ||
        picknplace_machine_uptime_ == DEFAULT_FLOAT_VALUE ||
        picknplace_technician_yearly_cost_ == DEFAULT_FLOAT_VALUE ||
        picknplace_time_ == DEFAULT_FLOAT_VALUE ||
        picknplace_group_ == DEFAULT_FLOAT_VALUE ||
        bonding_machine_cost_ == DEFAULT_FLOAT_VALUE ||
        bonding_machine_lifetime_ == DEFAULT_FLOAT_VALUE ||
        bonding_machine_uptime_ == DEFAULT_FLOAT_VALUE ||
        bonding_technician_yearly_cost_ == DEFAULT_FLOAT_VALUE ||
        bonding_time_ == DEFAULT_FLOAT_VALUE ||
        bonding_group_ == DEFAULT_FLOAT_VALUE ||
        die_separation_ == DEFAULT_FLOAT_VALUE ||
        edge_exclusion_ == DEFAULT_FLOAT_VALUE ||
        max_pad_current_density_ == DEFAULT_FLOAT_VALUE ||
        bonding_pitch_ == DEFAULT_FLOAT_VALUE ||
        alignment_yield_ == DEFAULT_FLOAT_VALUE ||
        bonding_yield_ == DEFAULT_FLOAT_VALUE ||
        dielectric_bond_defect_density_ == DEFAULT_FLOAT_VALUE) {
        return false;
    } else {
        return true;
    }
}

String Assembly::ToString() const {
    // Match the Python __str__ exactly
    String return_str = "Assembly Process Name: " + name_;
    return_str += "\n\r\tMaterials Cost Per mm^2: " + std::to_string(materials_cost_per_mm2_);
    return_str += "\n\r\tPick and Place Machine Cost: " + std::to_string(picknplace_machine_cost_);
    return_str += "\n\r\tPick and Place Machine Lifetime: " + std::to_string(picknplace_machine_lifetime_);
    return_str += "\n\r\tPick and Place Machine Uptime: " + std::to_string(picknplace_machine_uptime_);
    return_str += "\n\r\tPick and Place Technician Yearly Cost: " + std::to_string(picknplace_technician_yearly_cost_);
    return_str += "\n\r\tPick and Place Time: " + std::to_string(picknplace_time_);
    return_str += "\n\r\tPick and Place Group: " + std::to_string(picknplace_group_);
    return_str += "\n\r\tBonding Machine Cost: " + std::to_string(bonding_machine_cost_);
    return_str += "\n\r\tBonding Machine Lifetime: " + std::to_string(bonding_machine_lifetime_);
    return_str += "\n\r\tBonding Machine Uptime: " + std::to_string(bonding_machine_uptime_);
    return_str += "\n\r\tBonding Technician Yearly Cost: " + std::to_string(bonding_technician_yearly_cost_);
    return_str += "\n\r\tBonding Time: " + std::to_string(bonding_time_);
    return_str += "\n\r\tBonding Group: " + std::to_string(bonding_group_);
    return_str += "\n\r\tDie Separation: " + std::to_string(die_separation_);
    return_str += "\n\r\tEdge Exclusion: " + std::to_string(edge_exclusion_);
    return_str += "\n\r\tMax Pad Current Density: " + std::to_string(max_pad_current_density_);
    return_str += "\n\r\tBonding Pitch: " + std::to_string(bonding_pitch_);
    return_str += "\n\r\tAlignment Yield: " + std::to_string(alignment_yield_);
    return_str += "\n\r\tBonding Yield: " + std::to_string(bonding_yield_);
    return_str += "\n\r\tDielectric Bond Defect Density: " + std::to_string(dielectric_bond_defect_density_);
    return return_str;
}

FloatType Assembly::GetPowerPerPad(FloatType core_voltage) const {
    // Match the Python get_power_per_pad exactly
    FloatType pad_area = M_PI * std::pow(bonding_pitch_ / 4, 2);
    FloatType current_per_pad = max_pad_current_density_ * pad_area;
    FloatType power_per_pad = current_per_pad * core_voltage;
    return power_per_pad;
}

FloatType Assembly::ComputePicknplaceTime(IntType n_chips) const {
    // Match Python's compute_picknplace_time exactly
    IntType picknplace_steps = static_cast<IntType>(std::ceil(static_cast<FloatType>(n_chips) / picknplace_group_));
    FloatType time = picknplace_time_ * picknplace_steps;
    return time;
}

FloatType Assembly::ComputeBondingTime(IntType n_chips) const {
    // Match Python's compute_bonding_time exactly
    IntType bonding_steps = static_cast<IntType>(std::ceil(static_cast<FloatType>(n_chips) / bonding_group_));
    FloatType time = bonding_time_ * bonding_steps;
    return time;
}

FloatType Assembly::AssemblyTime(IntType n_chips) const {
    // Match Python's assembly_time exactly
    FloatType time = ComputePicknplaceTime(n_chips) + ComputeBondingTime(n_chips);
    return time;
}

FloatType Assembly::ComputePicknplaceCostPerSecond() {
    // Match Python's compute_picknplace_cost_per_second exactly
    if (bb_cost_per_second_ != DEFAULT_FLOAT_VALUE) {
        picknplace_cost_per_second_ = bb_cost_per_second_;
        return bb_cost_per_second_;
    }
    
    FloatType machine_cost_per_year = picknplace_machine_cost_ / picknplace_machine_lifetime_;
    FloatType technician_cost_per_year = picknplace_technician_yearly_cost_;
    FloatType picknplace_cost_per_year = machine_cost_per_year + technician_cost_per_year;
    FloatType picknplace_cost_per_second = (picknplace_cost_per_year / (365 * 24 * 60 * 60)) * picknplace_machine_uptime_;
    
    picknplace_cost_per_second_ = picknplace_cost_per_second;
    return picknplace_cost_per_second;
}

FloatType Assembly::ComputeBondingCostPerSecond() {
    // Match Python's compute_bonding_cost_per_second exactly
    if (bb_cost_per_second_ != DEFAULT_FLOAT_VALUE) {
        bonding_cost_per_second_ = bb_cost_per_second_;
        return bb_cost_per_second_;
    }
    
    FloatType machine_cost_per_year = bonding_machine_cost_ / bonding_machine_lifetime_;
    FloatType technician_cost_per_year = bonding_technician_yearly_cost_;
    FloatType bonding_cost_per_year = machine_cost_per_year + technician_cost_per_year;
    FloatType bonding_cost_per_second = (bonding_cost_per_year / (365 * 24 * 60 * 60)) * bonding_machine_uptime_;
    
    bonding_cost_per_second_ = bonding_cost_per_second;
    return bonding_cost_per_second;
}

FloatType Assembly::AssemblyCost(IntType n_chips, FloatType area) const {
    // Match Python's assembly_cost exactly
    FloatType assembly_cost = (picknplace_cost_per_second_ * ComputePicknplaceTime(n_chips) +
                              bonding_cost_per_second_ * ComputeBondingTime(n_chips));
    assembly_cost += materials_cost_per_mm2_ * area;
    return assembly_cost;
}

FloatType Assembly::AssemblyYield(IntType n_chips, IntType n_bonds, FloatType area) const {
    // Match Python's assembly_yield exactly
    FloatType assem_yield = 1.0;
    assem_yield *= python_pow(alignment_yield_, n_chips);
    assem_yield *= python_pow(bonding_yield_, n_bonds);
    
    // If hybrid bonding, there is some yield impact of the dielectric bond.
    // This uses a defect density and area number which approximates the negative binomial yield model with no clustering.
    FloatType dielectric_bond_area = area;
    FloatType denominator = 1.0 + (dielectric_bond_defect_density_ * dielectric_bond_area);
    FloatType dielectric_bond_yield = 1.0 / denominator;
    assem_yield *= dielectric_bond_yield;
    return assem_yield;
}

std::ostream& operator<<(std::ostream& os, const Assembly& assembly) {
    os << assembly.ToString();
    return os;
}

} // namespace design 