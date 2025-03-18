import design as d
import xml.etree.ElementTree as ET

wp_list = []
ap_list = []
tp_list = []
lp_list = []
io_list = []

wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
wp_list.append(wp)

ap = d.Assembly("test_assembly_process", 0.1, None, 1000000, 5, 0.9, 200000, 10,1,2000000,5,0.8,210000, 20,2,0.2,0.3,0.4,0.5,0.987,0.999,0.0003,True)
ap_list.append(ap)

tp = d.Test(name = "test_test_process", time_per_test_cycle = 0.000001, cost_per_second = 0.01, samples_per_input = 1, test_self = "False", bb_self_pattern_count = "", bb_self_scan_chain_length = "", self_defect_coverage = 0.9, self_test_reuse = 1, self_num_scan_chains = 7, self_num_io_per_scan_chain = 2, self_num_test_io_offset = 1, self_test_failure_dist = "normal", test_assembly = "False", bb_assembly_pattern_count = "", bb_assembly_scan_chain_length = "", assembly_defect_coverage = 0.5, assembly_test_reuse = 1, assembly_num_scan_chains = 3, assembly_num_io_per_scan_chain = 4, assembly_num_test_io_offset = 2, assembly_test_failure_dist = "normal", static = False)
tp_list.append(tp)

lp = d.Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, True)
lp_list.append(lp)

io = d.IO("test_io_process", 1.0, 2.0, 0.5, 3.0, 4, "True", 0.9, 5, True)
io_list.append(io)

chip_e = ET.Element("chip")
chip_e.set("name", "test_chip")
chip_e.set("bb_area", "")
chip_e.set("bb_cost", "")
chip_e.set("bb_power", "")
chip_e.set("bb_quality", "")
chip_e.set("aspect_ratio", "")
chip_e.set("x_location", "")
chip_e.set("y_location", "")
chip_e.set("fraction_memory", "0.2")
chip_e.set("fraction_logic", "0.5")
chip_e.set("fraction_analog", "0.3")
chip_e.set("gate_flop_ratio", "1.0")
chip_e.set("reticle_share", "1.0")
chip_e.set("buried", "False")
chip_e.set("assembly_process", "test_assembly_process")
chip_e.set("test_process", "test_test_process")
chip_e.set("stackup", "1:test_layer_process")
chip_e.set("wafer_process", "test_wafer_process")
chip_e.set("v_rail", "5")
chip_e.set("reg_eff", "1.0")
chip_e.set("reg_type", "none")
chip_e.set("core_voltage", "1.0")
chip_e.set("quantity", 1000000)
chip_e.set("core_area", "10.0")
chip_e.set("power", "3.0")

chip_etree = ET.ElementTree(chip_e)

adjacency_matrix = {}
matrix = [0]
adjacency_matrix["test_io_process"] = matrix


c = d.Chip(None, chip_etree.getroot(), None, wp_list, ap_list, tp_list, lp_list, io_list, adjacency_matrix, 0.5, [], False)


assert c.name == "test_chip"
assert c.core_area == 10.0
assert c.aspect_ratio == 1.0 
assert c.bb_area is None
assert c.bb_cost is None
assert c.bb_quality is None
assert c.y_location is None
assert c.x_location is None
assert c.bb_power is None
assert c.fraction_memory == 0.2
assert c.fraction_logic == 0.5
assert c.fraction_analog == 0.3
assert c.gate_flop_ratio == 1.0
assert c.reticle_share == 1.0
assert c.buried == False
assert c.chips == []
assert c.assembly_process.name == "test_assembly_process"
assert c.test_process.name == "test_test_process"
assert c.stackup[0].name == "test_layer_process"
assert c.wafer_process.name == "test_wafer_process"
assert c.core_voltage == 1.0
assert c.power == 3.0
assert c.quantity == 1000000



# Test the test class functions here:
assert c.test_process.compute_self_test_yield(c) == 1.0
assert c.test_process.compute_self_quality(c) == 0.9733930025109545
assert c.test_process.compute_assembly_test_yield(c) == 1.0
assert c.test_process.compute_assembly_quality(c) == 0.953822129670731
assert c.test_process.compute_self_pattern_count(c) == 2.8284271247461903
assert c.test_process.compute_self_scan_chain_length_per_mm2(c) == 1146.4285714285713
assert c.test_process.compute_self_test_cost(c) == 0.0
assert c.test_process.assembly_gate_flop_ratio(c) == 1.0
assert c.test_process.compute_assembly_pattern_count(c) == 2.8284271247461903
assert c.test_process.compute_assembly_scan_chain_length_per_mm2(c) == 267.49999999999994
assert c.test_process.compute_assembly_test_cost(c) == 0.0
assert c.test_process.num_test_ios() == 0
assert c.test_process.get_atpg_cost(c) == 0.0



# Test the chip class functions here:
assert c.compute_stack_power() == 0.0
assert c.get_parent_chip() == None
assert c.get_self_cost() == 13.206294120778358
assert c.get_cost() == 13.242294120778357

assert c.get_self_true_yield() == 0.9733930025109545
assert c.get_chip_true_yield() == 0.953822129670731
assert c.get_self_test_yield() == 1.0
assert c.get_chip_test_yield() == 1.0
assert c.get_self_gates_per_mm2() == 8024.999999999999
assert c.get_assembly_gates_per_mm2() == 802.4999999999999
assert c.get_self_quality() == 0.9733930025109545
assert c.get_quality() == 0.953822129670731
assert c.get_chips_len() == 0
assert c.get_chips() == []
assert c.get_wafer_diameter() == 234
assert c.get_edge_exclusion() == 1.2
assert c.get_wafer_process_yield() == 0.98
assert c.get_reticle_x() == 32
assert c.get_reticle_y() == 23
assert c.get_stack_power() == 0.0
assert c.get_io_power() == 0.0
assert c.get_total_power() == 3.0
assert c.get_area() == 81.0
assert c.get_stacked_die_area() == 0.36
assert c.compute_nre_front_end_cost() == 3.1999999999999993
assert c.compute_nre_back_end_cost() == 4.2
assert c.compute_nre_design_cost() == 7.3999999999999995

assert c.expandedArea(10,0.1,1.0) == 11.304911064067355
assert c.expandedArea(10,0.1,0.8) == 11.312792206135788
assert c.expandedArea(9,1.0,1.0) == 25.0
assert c.expandedArea(100,0.7,2.0) == 131.65848480983502

assert c.get_io_area() == 0
assert c.get_power_pads() == 306
assert c.get_pad_area() == 81.0
assert c.compute_area() == 81.0

assert c.compute_number_reticles(100)[0] == 1
assert c.compute_number_reticles(100)[1] == 0
assert c.compute_number_reticles(1000)[0] == 2
assert c.compute_number_reticles(1000)[1] == 1

assert c.compute_layer_aware_yield() == 0.9733930025109545
assert c.quality_yield() == 1.0
assert c.get_signal_count([])[0] == 0
assert c.get_signal_count([])[1] == {}

assert c.get_signal_power([]) == 0.0
assert c.get_chip_list()[0] == "test_chip"

assert c.get_chips_signal_count() == 0
assert c.compute_chip_yield() == 0.953822129670731
assert c.wafer_area_eff() == 42127.62651128383
assert c.get_layer_aware_cost() == 13.206294120778358
assert c.get_mask_cost() == 100000.0
assert c.compute_nre_cost() == 0.1000074
assert c.compute_self_cost() == 13.206294120778358
assert c.compute_cost() == 13.242294120778357
assert c.compute_self_perfect_yield_cost() == 13.206294120778358
assert c.compute_perfect_yield_cost() == 13.242294120778357
assert c.compute_scrap_cost() == 0.0
assert c.compute_total_non_scrap_cost() == 13.342301520778358
assert c.compute_total_cost() == 13.342301520778358

