import design as d

wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)

assert wp.wafer_fully_defined() == True
assert wp.name == "test_wafer_process"
assert wp.wafer_diameter == 234
assert wp.edge_exclusion == 1.2
assert wp.wafer_process_yield == 0.98
assert wp.dicing_distance == 0.87
assert wp.reticle_x == 32
assert wp.reticle_y == 23
assert wp.wafer_fill_grid == False
assert wp.nre_front_end_cost_per_mm2_memory == 0.1
assert wp.nre_back_end_cost_per_mm2_memory == 0.2
assert wp.nre_front_end_cost_per_mm2_logic == 0.3
assert wp.nre_back_end_cost_per_mm2_logic == 0.4
assert wp.nre_front_end_cost_per_mm2_analog == 0.5
assert wp.nre_back_end_cost_per_mm2_analog == 0.6

wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "True", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)

assert wp.wafer_fill_grid == True
