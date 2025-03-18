import design as d

tp = d.Test(name = "test_test_process", time_per_test_cycle = 0.000001, cost_per_second = 0.01, samples_per_input = 1, test_self = "False", bb_self_pattern_count = "", bb_self_scan_chain_length = "", self_defect_coverage = 0.9, self_test_reuse = 1, self_num_scan_chains = 7, self_num_io_per_scan_chain = 2, self_num_test_io_offset = 1, self_test_failure_dist = "normal", test_assembly = "False", bb_assembly_pattern_count = "", bb_assembly_scan_chain_length = "", assembly_defect_coverage = 0.5, assembly_test_reuse = 1, assembly_num_scan_chains = 3, assembly_num_io_per_scan_chain = 4, assembly_num_test_io_offset = 2, assembly_test_failure_dist = "normal", static = False)

assert tp.test_fully_defined() == True
assert tp.name == "test_test_process"
assert tp.time_per_test_cycle == 0.000001
assert tp.cost_per_second == 0.01
assert tp.samples_per_input == 1
assert tp.test_self == False
assert tp.bb_self_pattern_count is None
assert tp.bb_self_scan_chain_length is None
assert tp.self_defect_coverage == 0.9
assert tp.self_test_reuse == 1
assert tp.self_num_scan_chains == 7
assert tp.self_num_io_per_scan_chain == 2
assert tp.self_num_test_io_offset == 1
assert tp.self_test_failure_dist == "normal"
assert tp.test_assembly == False
assert tp.bb_assembly_pattern_count is None
assert tp.bb_assembly_scan_chain_length is None
assert tp.assembly_defect_coverage == 0.5
assert tp.assembly_test_reuse == 1
assert tp.assembly_num_scan_chains == 3
assert tp.assembly_num_io_per_scan_chain == 4
assert tp.assembly_num_test_io_offset == 2
assert tp.assembly_test_failure_dist == "normal"
