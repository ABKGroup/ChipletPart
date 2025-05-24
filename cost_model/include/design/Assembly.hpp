#ifndef ASSEMBLY_HPP
#define ASSEMBLY_HPP

#include "DesignCommon.hpp"
#include <iostream>
#include <cmath>

namespace design {

/**
 * @class Assembly
 * @brief Represents an assembly process with its parameters
 * 
 * The class has the following attributes:
 *   name: The name of the assembly process.
 *   materials_cost_per_mm2: The materials cost per mm^2 of the assembly process.
 *   bb_cost_per_second: The black box cost per second of assembly.
 *   picknplace_machine_cost: The cost of the pick and place machine.
 *   picknplace_machine_lifetime: The lifetime of the pick and place machine.
 *   picknplace_machine_uptime: The uptime of the pick and place machine.
 *   picknplace_technician_yearly_cost: The yearly cost of a pick and place technician.
 *   picknplace_time: The time to pick and place a chip.
 *   picknplace_group: The number of chips that can be picked and placed at once.
 *   bonding_machine_cost: The cost of the bonding machine.
 *   bonding_machine_lifetime: The lifetime of the bonding machine.
 *   bonding_machine_uptime: The uptime of the bonding machine.
 *   bonding_technician_yearly_cost: The yearly cost of a bonding technician.
 *   bonding_time: The time to bond a chip.
 *   bonding_group: The number of chips that can be bonded at once.
 *   die_separation: The distance between dies.
 *   edge_exclusion: The edge exclusion of the assembly process.
 *   max_pad_current_density: The maximum current density per pad.
 *   bonding_pitch: The pitch of bonding pads.
 *   alignment_yield: The yield of alignment.
 *   bonding_yield: The yield of bonding.
 *   dielectric_bond_defect_density: The defect density of dielectric bonding.
 *   picknplace_cost_per_second: Computed cost per second of pick and place.
 *   bonding_cost_per_second: Computed cost per second of bonding.
 *   static: A boolean set true when the assembly process is defined to prevent further changes.
 */
class Assembly {
public:
    /**
     * @brief Constructor for Assembly
     * 
     * @param name Name of the assembly process
     * @param materials_cost_per_mm2 Materials cost per mm^2
     * @param bb_cost_per_second Black box cost per second
     * @param picknplace_machine_cost Cost of pick and place machine
     * @param picknplace_machine_lifetime Lifetime of pick and place machine in years
     * @param picknplace_machine_uptime Uptime of pick and place machine (0-1)
     * @param picknplace_technician_yearly_cost Yearly cost of pick and place technician
     * @param picknplace_time Time to pick and place a chip in seconds
     * @param picknplace_group Number of chips that can be picked and placed at once
     * @param bonding_machine_cost Cost of bonding machine
     * @param bonding_machine_lifetime Lifetime of bonding machine in years
     * @param bonding_machine_uptime Uptime of bonding machine (0-1)
     * @param bonding_technician_yearly_cost Yearly cost of bonding technician
     * @param bonding_time Time to bond a chip in seconds
     * @param bonding_group Number of chips that can be bonded at once
     * @param die_separation Distance between dies in mm
     * @param edge_exclusion Edge exclusion in mm
     * @param max_pad_current_density Maximum current density per pad
     * @param bonding_pitch Pitch of bonding pads in mm
     * @param alignment_yield Yield of alignment (0-1)
     * @param bonding_yield Yield of bonding (0-1)
     * @param dielectric_bond_defect_density Defect density of dielectric bonding
     * @param static_value Whether the assembly process is static (unchangeable)
     */
    Assembly(
        const String& name = DEFAULT_STRING_VALUE,
        FloatType materials_cost_per_mm2 = DEFAULT_FLOAT_VALUE,
        FloatType bb_cost_per_second = DEFAULT_FLOAT_VALUE,
        FloatType picknplace_machine_cost = DEFAULT_FLOAT_VALUE,
        FloatType picknplace_machine_lifetime = DEFAULT_FLOAT_VALUE,
        FloatType picknplace_machine_uptime = DEFAULT_FLOAT_VALUE,
        FloatType picknplace_technician_yearly_cost = DEFAULT_FLOAT_VALUE,
        FloatType picknplace_time = DEFAULT_FLOAT_VALUE,
        FloatType picknplace_group = DEFAULT_FLOAT_VALUE,
        FloatType bonding_machine_cost = DEFAULT_FLOAT_VALUE,
        FloatType bonding_machine_lifetime = DEFAULT_FLOAT_VALUE,
        FloatType bonding_machine_uptime = DEFAULT_FLOAT_VALUE,
        FloatType bonding_technician_yearly_cost = DEFAULT_FLOAT_VALUE,
        FloatType bonding_time = DEFAULT_FLOAT_VALUE,
        FloatType bonding_group = DEFAULT_FLOAT_VALUE,
        FloatType die_separation = DEFAULT_FLOAT_VALUE,
        FloatType edge_exclusion = DEFAULT_FLOAT_VALUE,
        FloatType max_pad_current_density = DEFAULT_FLOAT_VALUE,
        FloatType bonding_pitch = DEFAULT_FLOAT_VALUE,
        FloatType alignment_yield = DEFAULT_FLOAT_VALUE,
        FloatType bonding_yield = DEFAULT_FLOAT_VALUE,
        FloatType dielectric_bond_defect_density = DEFAULT_FLOAT_VALUE,
        BoolType static_value = true);

    // Getters and setters for all properties
    String GetName() const;
    IntType SetName(const String& value);

    FloatType GetMaterialsCostPerMm2() const;
    IntType SetMaterialsCostPerMm2(FloatType value);

    FloatType GetBbCostPerSecond() const;
    IntType SetBbCostPerSecond(FloatType value);

    FloatType GetPicknplaceMachineCost() const;
    IntType SetPicknplaceMachineCost(FloatType value);

    FloatType GetPicknplaceMachineLifetime() const;
    IntType SetPicknplaceMachineLifetime(FloatType value);

    FloatType GetPicknplaceMachineUptime() const;
    IntType SetPicknplaceMachineUptime(FloatType value);

    FloatType GetPicknplaceTechnicianYearlyCost() const;
    IntType SetPicknplaceTechnicianYearlyCost(FloatType value);

    FloatType GetPicknplaceTime() const;
    IntType SetPicknplaceTime(FloatType value);

    FloatType GetPicknplaceGroup() const;
    IntType SetPicknplaceGroup(FloatType value);

    FloatType GetBondingMachineCost() const;
    IntType SetBondingMachineCost(FloatType value);

    FloatType GetBondingMachineLifetime() const;
    IntType SetBondingMachineLifetime(FloatType value);

    FloatType GetBondingMachineUptime() const;
    IntType SetBondingMachineUptime(FloatType value);

    FloatType GetBondingTechnicianYearlyCost() const;
    IntType SetBondingTechnicianYearlyCost(FloatType value);

    FloatType GetBondingTime() const;
    IntType SetBondingTime(FloatType value);

    FloatType GetBondingGroup() const;
    IntType SetBondingGroup(FloatType value);

    FloatType GetDieSeparation() const;
    IntType SetDieSeparation(FloatType value);

    FloatType GetEdgeExclusion() const;
    IntType SetEdgeExclusion(FloatType value);

    FloatType GetMaxPadCurrentDensity() const;
    IntType SetMaxPadCurrentDensity(FloatType value);

    FloatType GetBondingPitch() const;
    IntType SetBondingPitch(FloatType value);

    FloatType GetAlignmentYield() const;
    IntType SetAlignmentYield(FloatType value);

    FloatType GetBondingYield() const;
    IntType SetBondingYield(FloatType value);

    FloatType GetDielectricBondDefectDensity() const;
    IntType SetDielectricBondDefectDensity(FloatType value);

    FloatType GetPicknplaceCostPerSecond() const;
    IntType SetPicknplaceCostPerSecond(FloatType value);

    FloatType GetBondingCostPerSecond() const;
    IntType SetBondingCostPerSecond(FloatType value);

    BoolType GetStatic() const;
    void SetStatic(BoolType value);

    /**
     * @brief Check if all assembly parameters are defined
     * 
     * @return true if all parameters are defined, false otherwise
     */
    BoolType AssemblyFullyDefined() const;

    /**
     * @brief Set the assembly process as static (unchangeable)
     * 
     * @return 0 on success, 1 on failure
     */
    IntType SetStatic();

    /**
     * @brief Convert assembly process to string representation
     * 
     * @return String representation of the assembly process
     */
    String ToString() const;
    
    /**
     * @brief Get power per pad given core voltage
     * 
     * @param core_voltage Core voltage in volts
     * @return Power per pad in watts
     */
    FloatType GetPowerPerPad(FloatType core_voltage) const;
    
    /**
     * @brief Compute pick and place time for a given number of chips
     * 
     * @param n_chips Number of chips
     * @return Time in seconds
     */
    FloatType ComputePicknplaceTime(IntType n_chips) const;
    
    /**
     * @brief Compute bonding time for a given number of chips
     * 
     * @param n_chips Number of chips
     * @return Time in seconds
     */
    FloatType ComputeBondingTime(IntType n_chips) const;
    
    /**
     * @brief Calculate total assembly time for a given number of chips
     * 
     * @param n_chips Number of chips
     * @return Time in seconds
     */
    FloatType AssemblyTime(IntType n_chips) const;
    
    /**
     * @brief Compute pick and place cost per second
     * 
     * @return Cost per second
     */
    FloatType ComputePicknplaceCostPerSecond();
    
    /**
     * @brief Compute bonding cost per second
     * 
     * @return Cost per second
     */
    FloatType ComputeBondingCostPerSecond();
    
    /**
     * @brief Calculate assembly cost
     * 
     * @param n_chips Number of chips
     * @param area Area in mm^2
     * @return Total assembly cost
     */
    FloatType AssemblyCost(IntType n_chips, FloatType area) const;
    
    /**
     * @brief Calculate assembly yield
     * 
     * @param n_chips Number of chips
     * @param n_bonds Number of bonds
     * @param area Area in mm^2
     * @return Assembly yield
     */
    FloatType AssemblyYield(IntType n_chips, IntType n_bonds, FloatType area) const;
    
    // Operator overload for stream output
    friend std::ostream& operator<<(std::ostream& os, const Assembly& assembly);

private:
    // Private member variables (equivalent to Python's __variables)
    String name_;
    FloatType materials_cost_per_mm2_;
    FloatType bb_cost_per_second_;
    FloatType picknplace_machine_cost_;
    FloatType picknplace_machine_lifetime_;
    FloatType picknplace_machine_uptime_;
    FloatType picknplace_technician_yearly_cost_;
    FloatType picknplace_time_;
    FloatType picknplace_group_;
    FloatType bonding_machine_cost_;
    FloatType bonding_machine_lifetime_;
    FloatType bonding_machine_uptime_;
    FloatType bonding_technician_yearly_cost_;
    FloatType bonding_time_;
    FloatType bonding_group_;
    FloatType die_separation_;
    FloatType edge_exclusion_;
    FloatType max_pad_current_density_;
    FloatType bonding_pitch_;
    FloatType alignment_yield_;
    FloatType bonding_yield_;
    FloatType dielectric_bond_defect_density_;
    FloatType picknplace_cost_per_second_;
    FloatType bonding_cost_per_second_;
    BoolType static_;
};

} // namespace design

#endif // ASSEMBLY_HPP 