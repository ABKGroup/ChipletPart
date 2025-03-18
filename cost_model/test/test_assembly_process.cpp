#include <iostream>
#include <cassert>
#include <cmath>
#include "design/Assembly.hpp"

using namespace design;

// Helper function to check if a value is close to an expected value and report sign if it's not
bool check_close(double actual, double expected, double epsilon = 0.0001) {
    double diff = actual - expected;
    if (std::abs(diff) >= epsilon) {
        if (diff < 0) {
            std::cout << "FAIL: " << actual << " is less than expected " << expected << " by " << -diff << std::endl;
        } else {
            std::cout << "FAIL: " << actual << " is greater than expected " << expected << " by " << diff << std::endl;
        }
        return false;
    }
    return true;
}

int main() {
    // Create an Assembly with the same parameters as in the Python test
    Assembly ap(
        "test_assembly_process",    // name
        0.1,                        // materials_cost_per_mm2
        DEFAULT_FLOAT_VALUE,        // bb_cost_per_second (None in Python)
        1000000,                    // picknplace_machine_cost
        5,                          // picknplace_machine_lifetime
        0.9,                        // picknplace_machine_uptime
        200000,                     // picknplace_technician_yearly_cost
        10,                         // picknplace_time
        1,                          // picknplace_group
        2000000,                    // bonding_machine_cost
        5,                          // bonding_machine_lifetime
        0.8,                        // bonding_machine_uptime
        210000,                     // bonding_technician_yearly_cost
        20,                         // bonding_time
        2,                          // bonding_group
        0.2,                        // die_separation
        0.3,                        // edge_exclusion
        0.4,                        // max_pad_current_density
        0.5,                        // bonding_pitch
        0.987,                      // alignment_yield
        0.999,                      // bonding_yield
        0.0003,                     // dielectric_bond_defect_density
        true                        // static_value
    );
    
    // Test that all properties are correctly set
    assert(ap.AssemblyFullyDefined() == true);
    assert(ap.GetName() == "test_assembly_process");
    assert(check_close(ap.GetMaterialsCostPerMm2(), 0.1));
    assert(ap.GetBbCostPerSecond() == DEFAULT_FLOAT_VALUE); // None in Python
    assert(check_close(ap.GetPicknplaceMachineCost(), 1000000));
    assert(check_close(ap.GetPicknplaceMachineLifetime(), 5));
    assert(check_close(ap.GetPicknplaceMachineUptime(), 0.9));
    assert(check_close(ap.GetPicknplaceTechnicianYearlyCost(), 200000));
    assert(check_close(ap.GetPicknplaceTime(), 10));
    assert(check_close(ap.GetPicknplaceGroup(), 1));
    assert(check_close(ap.GetBondingMachineCost(), 2000000));
    assert(check_close(ap.GetBondingMachineLifetime(), 5));
    assert(check_close(ap.GetBondingMachineUptime(), 0.8));
    assert(check_close(ap.GetBondingTechnicianYearlyCost(), 210000));
    assert(check_close(ap.GetBondingTime(), 20));
    assert(check_close(ap.GetBondingGroup(), 2));
    assert(check_close(ap.GetDieSeparation(), 0.2));
    assert(check_close(ap.GetEdgeExclusion(), 0.3));
    assert(check_close(ap.GetMaxPadCurrentDensity(), 0.4));
    assert(check_close(ap.GetBondingPitch(), 0.5));
    assert(check_close(ap.GetAlignmentYield(), 0.987));
    assert(check_close(ap.GetBondingYield(), 0.999));
    assert(check_close(ap.GetDielectricBondDefectDensity(), 0.0003));
    
    // Test GetPowerPerPad
    assert(check_close(ap.GetPowerPerPad(1.0), 0.019634954084936207));
    assert(check_close(ap.GetPowerPerPad(0.8), 0.015707963267948967));
    assert(check_close(ap.GetPowerPerPad(1.2), 0.023561944901923447));
    assert(check_close(ap.GetPowerPerPad(10.0), 0.19634954084936207));
    
    // Test ComputePicknplaceTime
    assert(check_close(ap.ComputePicknplaceTime(1), 10));
    assert(check_close(ap.ComputePicknplaceTime(5), 50));
    assert(check_close(ap.ComputePicknplaceTime(10), 100));
    assert(check_close(ap.ComputePicknplaceTime(100), 1000));
    
    // Test ComputeBondingTime
    assert(check_close(ap.ComputeBondingTime(1), 20));
    assert(check_close(ap.ComputeBondingTime(5), 60));
    assert(check_close(ap.ComputeBondingTime(10), 100));
    assert(check_close(ap.ComputeBondingTime(100), 1000));
    
    // Test AssemblyTime
    assert(check_close(ap.AssemblyTime(1), 30));
    assert(check_close(ap.AssemblyTime(5), 110));
    assert(check_close(ap.AssemblyTime(10), 200));
    assert(check_close(ap.AssemblyTime(100), 2000));
    
    // Test picknplace_cost_per_second
    // In the C++ version, we need to call ComputePicknplaceCostPerSecond first
    // because the property isn't automatically calculated
    ap.ComputePicknplaceCostPerSecond();
    assert(check_close(ap.GetPicknplaceCostPerSecond(), 0.01141552511415525));
    
    // Test bonding_cost_per_second
    // In the C++ version, we need to call ComputeBondingCostPerSecond first
    ap.ComputeBondingCostPerSecond();
    assert(check_close(ap.GetBondingCostPerSecond(), 0.015474378488077117));
    
    // Test AssemblyCost
    assert(check_close(ap.AssemblyCost(1, 10), 1.423642820903095));
    assert(check_close(ap.AssemblyCost(100, 10), 27.88990360223237));
    assert(check_close(ap.AssemblyCost(1, 50), 5.423642820903095));
    assert(check_close(ap.AssemblyCost(70, 33), 22.122932521562657));
    
    // Test AssemblyYield
    assert(check_close(ap.AssemblyYield(1, 1, 10), 0.9830638085742773));
    assert(check_close(ap.AssemblyYield(10, 1, 10), 0.873848372866451));
    assert(check_close(ap.AssemblyYield(100, 1, 10), 0.2691409754969558));
    assert(check_close(ap.AssemblyYield(1, 1000, 10), 0.3618298945652455));
    assert(check_close(ap.AssemblyYield(10, 1000, 10), 0.3216316803268721));
    assert(check_close(ap.AssemblyYield(100, 1000, 10), 0.0990609662748996));
    assert(check_close(ap.AssemblyYield(1, 1, 9), 0.983357933579336));
    assert(check_close(ap.AssemblyYield(10, 1, 15), 0.8725434723594329));
    assert(check_close(ap.AssemblyYield(100, 1, 70), 0.2643960807281554));
    assert(check_close(ap.AssemblyYield(1, 1000, 100), 0.3523450332513992));
    assert(check_close(ap.AssemblyYield(10, 1000, 200), 0.30433639185646477));
    assert(check_close(ap.AssemblyYield(100, 1000, 400), 0.08871263319082526));
    
    std::cout << "All Assembly tests passed!" << std::endl;
    return 0;
} 