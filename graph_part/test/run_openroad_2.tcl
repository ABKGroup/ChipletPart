set num_parts 2
set balance 5
set seed 0
set hypergraph_file "/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/graph_part/testcases/48_1_14_4_1600_1600/block_level_netlist_ws-48_1_14_4_1600_1600_hmetis.hgr"
set hypergraph_file "/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/graph_part/testcases/mempool_group_100_100/block_level_netlist_mempool_group_100_100_hmetis.hgr"
triton_part_hypergraph -hypergraph_file $hypergraph_file -num_parts $num_parts -balance $balance -seed $seed

exit