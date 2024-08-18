set num_parts 8
set balance 5
set seed 0
set hypergraph_file "/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/graph_part/testcases/48_4_14_4_400_1/block_level_netlist_ws-48_4_14_4_400_1_hmetis.hgr"

triton_part_hypergraph -hypergraph_file $hypergraph_file -num_parts $num_parts -balance $balance -seed $seed