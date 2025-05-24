#include <iostream>
#include <cassert>
#include <cmath>
#include <memory>
#include <map>
#include <vector>
#include "design/Chip.hpp"
#include "design/WaferProcess.hpp"
#include "design/Assembly.hpp"
#include "design/Test.hpp"
#include "design/Layer.hpp"
#include "design/IO.hpp"
#include "pugixml.hpp"

using namespace design;
using namespace pugi;

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
    // Create all needed process objects
    std::vector<WaferProcess> wp_list;
    std::vector<Assembly> ap_list;
    std::vector<Test> tp_list;
    std::vector<Layer> lp_list;
    std::vector<IO> io_list;

    // Create a WaferProcess
    WaferProcess wp("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6);
    wp_list.push_back(wp);

    std::cout << "WaferProcess created" << std::endl;

    // Create an Assembly
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
    ap_list.push_back(ap);

    std::cout << "Assembly created" << std::endl;

    // Create a Test
    Test tp(
        "test_test_process",    // name
        0.000001,               // time_per_test_cycle
        0.01,                   // cost_per_second
        1,                      // samples_per_input
        "False",                // test_self
        DEFAULT_FLOAT_VALUE,    // bb_self_pattern_count
        DEFAULT_FLOAT_VALUE,    // bb_self_scan_chain_length
        0.9,                    // self_defect_coverage
        1,                      // self_test_reuse
        7,                      // self_num_scan_chains
        2,                      // self_num_io_per_scan_chain
        1,                      // self_num_test_io_offset
        "normal",               // self_test_failure_dist
        "False",                // test_assembly
        DEFAULT_FLOAT_VALUE,    // bb_assembly_pattern_count
        DEFAULT_FLOAT_VALUE,    // bb_assembly_scan_chain_length
        0.5,                    // assembly_defect_coverage
        1,                      // assembly_test_reuse
        3,                      // assembly_num_scan_chains
        4,                      // assembly_num_io_per_scan_chain
        2,                      // assembly_num_test_io_offset
        "normal",               // assembly_test_failure_dist
        false                   // static_value
    );
    tp_list.push_back(tp);

    std::cout << "Test created" << std::endl;

    // Create a Layer
    Layer lp("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, true);
    lp_list.push_back(lp);

    std::cout << "Layer created" << std::endl;

    // Create an IO - make sure all parameters are explicitly set to non-default values
    IO io("test_io_process", 1.0, 2.0, 0.5, 3.0, 4, "True", 0.9, 5.0, true);
    
    // Verify that the IO is fully defined before adding it to the list
    if (!io.IoFullyDefined()) {
        std::cout << "Error: IO is not fully defined. Debug information:" << std::endl;
        std::cout << io.ToString() << std::endl;
    }
    assert(io.IoFullyDefined() == true);
    
    io_list.push_back(io);

    // Create an XML element for the chip
    xml_document doc;
    xml_node chip_e = doc.append_child("chip");
    chip_e.append_attribute("name") = "test_chip";
    chip_e.append_attribute("bb_area") = "";
    chip_e.append_attribute("bb_cost") = "";
    chip_e.append_attribute("bb_power") = "";
    chip_e.append_attribute("bb_quality") = "";
    chip_e.append_attribute("aspect_ratio") = "";
    chip_e.append_attribute("x_location") = "";
    chip_e.append_attribute("y_location") = "";
    chip_e.append_attribute("fraction_memory") = "0.2";
    chip_e.append_attribute("fraction_logic") = "0.5";
    chip_e.append_attribute("fraction_analog") = "0.3";
    chip_e.append_attribute("gate_flop_ratio") = "1.0";
    chip_e.append_attribute("reticle_share") = "1.0";
    chip_e.append_attribute("buried") = "False";
    chip_e.append_attribute("assembly_process") = "test_assembly_process";
    chip_e.append_attribute("test_process") = "test_test_process";
    chip_e.append_attribute("stackup") = "1:test_layer_process";
    chip_e.append_attribute("wafer_process") = "test_wafer_process";
    chip_e.append_attribute("v_rail") = "5";
    chip_e.append_attribute("reg_eff") = "1.0";
    chip_e.append_attribute("reg_type") = "none";
    chip_e.append_attribute("core_voltage") = "1.0";
    chip_e.append_attribute("quantity") = "1000000";
    chip_e.append_attribute("core_area") = "10.0";
    chip_e.append_attribute("power") = "3.0";

    // Create adjacency matrix
    std::map<String, std::vector<std::vector<IntType>>> adjacency_matrix;
    std::vector<std::vector<IntType>> matrix;
    matrix.push_back({0});
    adjacency_matrix["test_io_process"] = matrix;

    // Create an empty average bandwidth utilization map
    std::map<String, std::vector<std::vector<FloatType>>> avg_bandwidth_util;
    std::vector<std::vector<FloatType>> bw_matrix;
    bw_matrix.push_back({0.5});
    avg_bandwidth_util["test_io_process"] = bw_matrix;

    // Create an empty block_names vector
    std::vector<String> block_names;

    // Create the Chip instance
    std::shared_ptr<Chip> c = std::make_shared<Chip>(
        "",              // filename (null in Python)
        &chip_e,         // etree
        nullptr,         // parent_chip (null in Python)
        &wp_list,        // wafer_process_list
        &ap_list,        // assembly_process_list
        &tp_list,        // test_process_list
        &lp_list,        // layers
        &io_list,        // ios
        &adjacency_matrix, // adjacency_matrix_definitions
        &avg_bandwidth_util, // average_bandwidth_utilization (empty map in Python)
        &block_names,    // block_names (empty list in Python)
        false            // static_value
    );

    std::cout << "Chip created" << std::endl;
    
    // No longer need to set up Test functions for the Chip
    
    // Now initialize the Chip
    c->Initialize();
    std::cout << "Chip initialized" << std::endl;

    // Test the Chip properties
    assert(c->GetName() == "test_chip");
    assert(check_close(c->GetCoreArea(), 10.0));
    assert(check_close(c->GetAspectRatio(), 1.0));
    assert(c->GetBbArea() == DEFAULT_FLOAT_VALUE);  // None in Python
    assert(c->GetBbCost() == DEFAULT_FLOAT_VALUE);  // None in Python
    assert(c->GetBbQuality() == DEFAULT_FLOAT_VALUE);  // None in Python
    assert(c->GetYLocation() == DEFAULT_FLOAT_VALUE);  // None in Python
    assert(c->GetXLocation() == DEFAULT_FLOAT_VALUE);  // None in Python
    assert(c->GetBbPower() == DEFAULT_FLOAT_VALUE);  // None in Python
    assert(check_close(c->GetFractionMemory(), 0.2));
    assert(check_close(c->GetFractionLogic(), 0.5));
    assert(check_close(c->GetFractionAnalog(), 0.3));
    assert(check_close(c->GetGateFlopRatio(), 1.0));
    assert(check_close(c->GetReticleShare(), 1.0));
    assert(c->GetBuried() == false);
    assert(c->GetChips().empty());
    assert(c->GetAssemblyProcess()->GetName() == "test_assembly_process");
    assert(c->GetTestProcess()->GetName() == "test_test_process");
    assert(c->GetStackup()[0].GetName() == "test_layer_process");
    assert(c->GetWaferProcess()->GetName() == "test_wafer_process");
    assert(check_close(c->GetCoreVoltage(), 1.0));
    assert(check_close(c->GetPower(), 3.0));
    assert(c->GetQuantity() == 1000000);

    // Test the test class functions
    assert(check_close(c->GetTestProcess()->ComputeSelfTestYield(c->GetSelfTrueYield(), c->GetName()), 1.0));
    assert(check_close(c->GetTestProcess()->ComputeSelfQuality(c->GetSelfTrueYield(), c->GetSelfTestYield()), 0.9733930025109545));
    assert(check_close(c->GetTestProcess()->ComputeAssemblyTestYield(c->GetChipTrueYield()), 1.0));
    assert(check_close(c->GetTestProcess()->ComputeAssemblyQuality(c->GetChipTrueYield(), c->GetChipTestYield()), 0.9539251424607355));
    assert(check_close(c->GetTestProcess()->ComputeSelfPatternCount(c->GetCoreArea(), c->GetSelfGatesPerMm2(), c->GetGateFlopRatio()), 2.8284271247461903));
    assert(check_close(c->GetTestProcess()->ComputeSelfScanChainLengthPerMm2(c->GetCoreArea(), c->GetSelfGatesPerMm2(), c->GetGateFlopRatio()), 1146.4285714285713));
    assert(check_close(c->GetTestProcess()->ComputeSelfTestCost(c->GetCoreArea(), c->GetSelfGatesPerMm2(), c->GetGateFlopRatio()), 0.0));
    assert(check_close(c->GetTestProcess()->AssemblyGateFlopRatio(c->GetCoreArea(), c->GetAssemblyGatesPerMm2()), 1.0));
    assert(check_close(c->GetTestProcess()->ComputeAssemblyPatternCount(c->GetCoreArea(), c->GetAssemblyGatesPerMm2()), 2.8284271247461903));
    assert(check_close(c->GetTestProcess()->ComputeAssemblyScanChainLengthPerMm2(c->GetCoreArea(), c->GetAssemblyGatesPerMm2()), 267.49999999999994));
    assert(check_close(c->GetTestProcess()->ComputeAssemblyTestCost(c->GetCoreArea(), c->GetAssemblyGatesPerMm2()), 0.0));
    assert(check_close(c->GetTestProcess()->NumTestIos(), 0.0));
    assert(check_close(c->GetTestProcess()->GetAtpgCost(c->GetCoreArea(), c->GetCoreArea(), c->GetSelfGatesPerMm2(), c->GetAssemblyGatesPerMm2(), c->GetGateFlopRatio()), 0.0));

    // Test the chip class functions
    assert(check_close(c->ComputeStackPower(), 0.0));
    assert(c->GetParentChip() == nullptr);
    assert(check_close(c->GetSelfCost(), 13.206294120778358));
    assert(check_close(c->GetCost(), 13.206294120778358));

    assert(check_close(c->GetSelfTrueYield(), 0.9733930025109545));
    assert(check_close(c->GetChipTrueYield(), 0.9539251424607355));
    assert(check_close(c->GetSelfTestYield(), 1.0));
    assert(check_close(c->GetChipTestYield(), 1.0));
    assert(check_close(c->GetSelfGatesPerMm2(), 8024.999999999999));
    assert(check_close(c->GetAssemblyGatesPerMm2(), 802.4999999999999));
    assert(check_close(c->GetSelfQuality(), 0.9733930025109545));
    std::cout << "Testing GetQuality" << std::endl;
    assert(check_close(c->GetQuality(), 0.9539251424607355));
    assert(c->GetChipsLen() == 0);
    assert(c->GetChips().empty());
    assert(check_close(c->GetWaferDiameter(), 234));
    assert(check_close(c->GetEdgeExclusion(), 1.2));
    assert(check_close(c->GetWaferProcessYield(), 0.98));
    assert(check_close(c->GetReticleX(), 32));
    assert(check_close(c->GetReticleY(), 23));
    assert(check_close(c->GetStackPower(), 0.0));
    assert(check_close(c->GetIoPower(), 0.0));
    assert(check_close(c->GetTotalPower(), 3.0));
    assert(check_close(c->GetArea(), 81.0));
    assert(check_close(c->GetStackedDieArea(), 0.0));
    assert(check_close(c->ComputeNreFrontEndCost(), 3.1999999999999993));
    assert(check_close(c->ComputeNreBackEndCost(), 4.2));
    assert(check_close(c->ComputeNreDesignCost(), 7.3999999999999995));

    assert(check_close(c->ExpandedArea(10, 0.1, 1.0), 11.304911064067355));
    assert(check_close(c->ExpandedArea(10, 0.1, 0.8), 11.312792206135788));
    assert(check_close(c->ExpandedArea(9, 1.0, 1.0), 25.0));
    assert(check_close(c->ExpandedArea(100, 0.7, 2.0), 131.65848480983502));

    assert(check_close(c->GetIoArea(), 0.0));
    assert(c->GetPowerPads() == 306);
    assert(check_close(c->GetPadArea(), 81.0));
    assert(check_close(c->ComputeArea(), 81.0));

    auto reticles_result = c->ComputeNumberReticles(100);
    assert(reticles_result.first == 1);  // number of reticles
    assert(reticles_result.second == 0); // number of stitches
    reticles_result = c->ComputeNumberReticles(1000);
    assert(reticles_result.first == 2);  // number of reticles
    assert(reticles_result.second == 1); // number of stitches

    assert(check_close(c->ComputeLayerAwareYield(), 0.9733930025109545));
    assert(check_close(c->QualityYield(), 1.0));
    
    auto signal_count_result = c->GetSignalCount({});
    assert(signal_count_result.first == 0); // total signal count
    assert(signal_count_result.second.empty()); // signal count map

    assert(check_close(c->GetSignalPower({}), 0.0));
    
    auto chip_list = c->GetChipList();
    assert(chip_list.size() == 1);
    assert(chip_list[0] == "test_chip");

    assert(c->GetChipsSignalCount() == 0);
    assert(check_close(c->ComputeChipYield(), 0.9539251424607355));
    assert(check_close(c->WaferAreaEff(), 42127.62651128383));
    assert(check_close(c->GetLayerAwareCost(), 13.206294120778358));
    assert(check_close(c->GetMaskCost(), 100000.0));
    assert(check_close(c->ComputeNreCost(), 0.1000074));
    assert(check_close(c->ComputeSelfCost(), 13.206294120778358));
    assert(check_close(c->ComputeCost(), 13.20629412077835));
    assert(check_close(c->ComputeSelfPerfectYieldCost(), 13.206294120778358));
    assert(check_close(c->ComputePerfectYieldCost(), 13.206294120778358));
    assert(check_close(c->ComputeScrapCost(), 0.0));
    assert(check_close(c->ComputeTotalNonScrapCost(), 13.306301520778359));
    assert(check_close(c->ComputeTotalCost(), 13.30630152077835));

    std::cout << "All Chip tests passed!" << std::endl;
    return 0;
} 