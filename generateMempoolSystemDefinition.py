# This is a testcase generator for the mempool design.
# This supports generating the mempool testcase for a single mempool group.
# The interconnect is scaled based on Rent's rule coefficients.

import numpy as np
import sys
import math

io_cell_name = "2Gbs_100vCDM_2mm"
assumed_bandwidth_per_wire = 4 # (Gbps) Assumed bandwidth per wire in the interconnect.
tech_node = "45nm"
total_power = 1.55/4 # (Watts) Will be divided among blocks according to area. (1.55 is for the total 4-group case.)

rents_rule_alpha = 0.45 # Assuming the exponent from Optimal Chip Sizing for Multi-Chip Modules for Microprossecors

# Generate a block definition file with the format: block_name block_area block_power
def generate_block_definitions(block_area_file,area_multiplier,power_multiplier):
    block_definitions_filename = "block_definitions_mempool_group_" + str(area_multiplier) + "_" + str(power_multiplier) + ".txt"

    total_area = 0.0
    blocks = []
    areas = []
    powers = []
    with open(block_area_file, "r") as f:
        for line in f:
            stripped_split = line.strip().split()
            blocks.append(stripped_split[0])
            area = float(stripped_split[1])
            areas.append(area)
            total_area += area

    for area in areas:
        power = total_power*(area/total_area)
        powers.append(power)

    with open(block_definitions_filename, 'w') as f:
        for i in range(len(blocks)):
            f.write(blocks[i] + " " + str(areas[i]*area_multiplier) + " " + str(powers[i]*power_multiplier) + " " + tech_node + "\n")

    return block_definitions_filename

# Define a function to take a string and a list of strings and return the index of the string in the list.
def get_index(s, l):
    for i in range(len(l)):
        if l[i] == s:
            return i
    return -1

# Generate a block level netlist file with the pairwise block xml format.
def generate_block_level_netlist(block_area_file,netlist_file,area_multiplier,power_multiplier):
    block_level_netlist_filename = "block_level_netlist_mempool_group_" + str(area_multiplier) + "_" + str(power_multiplier) + ".xml"

    blocks = []
    with open(block_area_file, "r") as f:
        for line in f:
            blocks.append(line.strip().split()[0])

    n = len(blocks)
    adj = np.zeros((n,n))

    # Read the nets file and update the adjacency matrix.
    with open(netlist_file, "r") as f:
        for line in f:
            split_blocks = line.strip().split()
            from_block = split_blocks[0]
            to_blocks = split_blocks[1:]
            from_index = get_index(from_block, blocks)
            if from_index != -1:
                for to_block in to_blocks:
                    to_index = get_index(to_block, blocks)
                    if to_index != -1 and from_index != to_index:
                        adj[from_index][to_index] += 1


    netlist = []
    netlist.append("<netlist>")

    for x in range(n):
        for y in range(n):
            if adj[x][y] > 0:
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"" + blocks[x] + "\"\n\t\tblock1=\"" + blocks[y] + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(math.ceil(adj[x][y]*(area_multiplier**rents_rule_alpha)/assumed_bandwidth_per_wire)) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")

    netlist.append("</netlist>")

    # Write the netlist to a file
    with open(block_level_netlist_filename, 'w') as f:
        for line in netlist:
            f.write(line + "\n")

    return block_level_netlist_filename


# Main function
def main():
    # Inputs from the command line:
    #   1. Area multiplier
    #   2. Power multiplier

    if (len(sys.argv) != 3):
        print("Error: Incorrect number of input arguments.")
        print("Correct Usage: python generateMempoolSystemDefinition.py <area_multiplier> <power_multiplier>")
        print("Exiting...")
        exit()


    nets_file = "nets.txt"
    blocks_file = "second_level_mods.txt"

    # The nets file contains a two or more block names per line, separated by spaces.
    # The first block name is the from block, and the remaining block names are the to blocks.
    # The blocks file contains a list of block names, one per line.

    # First, read all the block names from the blocks file.
    # Then store the names in a numpy array (the length of the array is n) and create a 2D adjacency matrix with size (n,n) initialized to all zeros.
    blocks = []
    with open(blocks_file, "r") as f:
        for line in f:
            blocks.append(line.strip().split()[0])

    n = len(blocks)
    adj = np.zeros((n,n))

    # Read the nets file and update the adjacency matrix.
    with open(nets_file, "r") as f:
        for line in f:
            split_blocks = line.strip().split()
            from_block = split_blocks[0]
            to_blocks = split_blocks[1:]
            from_index = get_index(from_block, blocks)
            if from_index != -1:
                for to_block in to_blocks:
                    to_index = get_index(to_block, blocks)
                    if to_index != -1 and from_index != to_index:
                        adj[from_index][to_index] += 1

    # Print some statistics on the adjacency matrix.
    print("Number of blocks:", n)
    print("Number of edges:", int(np.sum(adj)))
    print("Number of non-zero entries in the adjacency matrix:", np.count_nonzero(adj))
    print("Number of blocks with no incoming edges:", np.count_nonzero(np.sum(adj, axis=0) == 0))
    print("Number of blocks with no outgoing edges:", np.count_nonzero(np.sum(adj, axis=1) == 0))
    print("Number of blocks with self-loops:", np.count_nonzero(np.diag(adj) > 0))
    print("Number of blocks with multiple incoming edges:", np.count_nonzero(np.sum(adj, axis=0) > 1))
    print("Number of blocks with multiple outgoing edges:", np.count_nonzero(np.sum(adj, axis=1) > 1))
    dims = adj.shape
    for x in range(dims[0]):
        print(blocks[x] + "\t", end="")
        for y in range(dims[1]):
            print(str(adj[x,y]) + " ",end="")
        print("\n",end="")
    print("Total number of edges: " + str(adj.sum()))

    area_multiplier = int(sys.argv[1])
    power_multiplier = int(sys.argv[2])

    block_def_file = generate_block_definitions(blocks_file,area_multiplier,power_multiplier)
    if block_def_file:
        print("Block Definition File Generated: " + block_def_file)

    block_level_netlist_file = generate_block_level_netlist(blocks_file,nets_file,area_multiplier,power_multiplier)
    if block_level_netlist_file:
        print("Block Level Netlist File Generated: " + block_level_netlist_file)

    return


if __name__ == '__main__':
    main()