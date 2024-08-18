# This is a testcase generator for the waferscale design.
# This supports 3 different configurations: ws-48, ws-3, and ws-1.
# The number indicates the number of IPs in a tile for each testcase.

import numpy as np
import sys
import math

valid_configurations = ["ws-48","ws-3","ws-1"]
io_cell_name = "2Gbs_100vCDM_2mm"
tech_node = "45nm"

rents_rule_alpha = 0.45 # Assuming the exponent from Optimal Chip Sizing for Multi-Chip Modules for Microprossecors

# Generate a block definition file with the format: block_name block_area block_power
def generate_block_definitions(ws_configuration,num_tiles,num_cores_per_tile,num_shared_memories_per_tile,area_multiplier,power_multiplier):
    block_definitions_filename = "block_definitions_" + ws_configuration + "_" + str(num_tiles) + "_" + str(num_cores_per_tile) + "_" + str(num_shared_memories_per_tile) + "_" + str(area_multiplier) + "_" + str(power_multiplier) + ".txt"

    # For the ws-48 case
    master_crossbar_area = 0.041655*area_multiplier
    master_crossbar_power = 0.03*power_multiplier
    router_area = 0.043343*area_multiplier
    router_power = 0.02*power_multiplier
    shared_memory_area = 0.102287*area_multiplier
    shared_memory_power = 0.015*power_multiplier
    core_area = 0.011705*area_multiplier
    core_power = 0.000785*power_multiplier
    bus_mux_area = 0.003286*area_multiplier
    bus_mux_power = 0.000185*power_multiplier
    private_memory_area = 0.020359*area_multiplier
    private_memory_power = 0.0002*power_multiplier

    # For the ws-3 case
    network_area = master_crossbar_area + router_area
    network_power = master_crossbar_power + router_power
    memory_area = shared_memory_area*num_shared_memories_per_tile
    memory_power = shared_memory_power*num_shared_memories_per_tile
    compute_area = (core_area + bus_mux_area + private_memory_area) * num_cores_per_tile
    compute_power = (core_power + bus_mux_power + private_memory_power) * num_cores_per_tile

    # For the ws-1 case
    total_area = network_area + memory_area + compute_area
    total_power = network_power + memory_power + compute_power


    # Generate the line for each block and store in a list
    block_list = []
    for tile in range(0,num_tiles):
        if ws_configuration == "ws-48":
            # Master Crossbar
            block_list.append("Master_Crossbar_" + str(tile) + " " + str(master_crossbar_area) + " " + str(master_crossbar_power) + " " + tech_node)
            # Router
            block_list.append("Router_" + str(tile) + " " + str(router_area) + " " + str(router_power) + " " + tech_node)
            # Shared Memories
            for shared_memory in range(0,num_shared_memories_per_tile):
                block_list.append("Shared_Memory_" + str(tile) + "_" + str(shared_memory) + " " + str(shared_memory_area) + " " + str(shared_memory_power) + " " + tech_node)
            # Core Groups
            for core in range(0,num_cores_per_tile):
                # Cores
                block_list.append("Core_" + str(tile) + "_" + str(core) + " " + str(core_area) + " " + str(core_power) + " " + tech_node)
                # Bus Muxes
                block_list.append("Bus_Mux_" + str(tile) + "_" + str(core) + " " + str(bus_mux_area) + " " + str(bus_mux_power) + " " + tech_node)
                # Private Memory
                block_list.append("Private_Memory_" + str(tile) + "_" + str(core) + " " + str(private_memory_area) + " " + str(private_memory_power) + " " + tech_node)
        elif ws_configuration == "ws-3":
            # Network
            block_list.append("Network_" + str(tile) + " " + str(network_area) + " " + str(network_power) + " " + tech_node)
            # Memory
            block_list.append("Memory_" + str(tile) + " " + str(memory_area) + " " + str(memory_power) + " " + tech_node)
            # Compute
            block_list.append("Compute_" + str(tile) + " " + str(compute_area) + " " + str(compute_power) + " " + tech_node)
        elif ws_configuration == "ws-1":
            block_list.append("Tile_" + str(tile) + " " + str(total_area) + " " + str(total_power) + " " + tech_node)

    # Write the block list to a file
    with open(block_definitions_filename, 'w') as f:
        for block in block_list:
            f.write(block + "\n")

    return block_definitions_filename


# Generate a block level netlist file with the pairwise block xml format.
def generate_block_level_netlist(ws_configuration,num_tiles,num_cores_per_tile,num_shared_memories_per_tile,area_multiplier,power_multiplier):
    block_level_netlist_filename = "block_level_netlist_" + ws_configuration + "_" + str(num_tiles) + "_" + str(num_cores_per_tile) + "_" + str(num_shared_memories_per_tile) + "_" + str(area_multiplier) + "_" + str(power_multiplier) + ".xml"

    netlist = []
    netlist.append("<netlist>")
    
    rents_rule_scaling = area_multiplier**rents_rule_alpha

    for tile in range(0,num_tiles):
        if ws_configuration == "ws-48":
            # Master_Crossbar to Router 196, Router to Master_Crossbar 194 wires
            netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Master_Crossbar_" + str(tile) + "\"\n\t\tblock1=\"Router_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*196) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
            netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Router_" + str(tile) + "\"\n\t\tblock1=\"Master_Crossbar_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*194) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")

            # Master_Crossbar to Shared Memory 132, Shared Memory to Master_Crossbar 64 wires
            for shared_memory in range(0,num_shared_memories_per_tile):
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Master_Crossbar_" + str(tile) + "\"\n\t\tblock1=\"Shared_Memory_" + str(tile) + "_" + str(shared_memory) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*132) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Shared_Memory_" + str(tile) + "_" + str(shared_memory) + "\"\n\t\tblock1=\"Master_Crossbar_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*64) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")

            for core in range(0,num_cores_per_tile):
                # Master_Crossbar to Core 64, Core to Master_Crossbar 130 wires
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Master_Crossbar_" + str(tile) + "\"\n\t\tblock1=\"Core_" + str(tile) + "_" + str(core) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*64) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Core_" + str(tile) + "_" + str(core) + "\"\n\t\tblock1=\"Master_Crossbar_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*130) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                # Core to Bus Mux 280, Bus Mux to Core 64 wires
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Core_" + str(tile) + "_" + str(core) + "\"\n\t\tblock1=\"Bus_Mux_" + str(tile) + "_" + str(core) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*280) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Bus_Mux_" + str(tile) + "_" + str(core) + "\"\n\t\tblock1=\"Core_" + str(tile) + "_" + str(core) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*64) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                # Bus Mux to Private Memory 64, Private Memory to Bus Mux 110 wires
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Bus_Mux_" + str(tile) + "_" + str(core) + "\"\n\t\tblock1=\"Private_Memory_" + str(tile) + "_" + str(core) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*64) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Private_Memory_" + str(tile) + "_" + str(core) + "\"\n\t\tblock1=\"Bus_Mux_" + str(tile) + "_" + str(core) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*110) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
            
        elif ws_configuration == "ws-3":
            # Network to Memory 132*num_shared_memories_per_tile, Memory to Network 64*num_shared_memories_per_tile wires
            netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Network_" + str(tile) + "\"\n\t\tblock1=\"Memory_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(132*num_shared_memories_per_tile*rents_rule_scaling) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
            netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Memory_" + str(tile) + "\"\n\t\tblock1=\"Network_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(64*num_shared_memories_per_tile*rents_rule_scaling) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
            # Network to Compute 64*num_cores_per_tile, Compute to Network 130*num_cores_per_tile wires
            netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Network_" + str(tile) + "\"\n\t\tblock1=\"Compute_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(64*num_cores_per_tile*rents_rule_scaling) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
            netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Compute_" + str(tile) + "\"\n\t\tblock1=\"Network_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(130*num_cores_per_tile*rents_rule_scaling) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
        elif ws_configuration == "ws-1":
            # No internal connections for ws-1
            pass

    # Connect Routers
    # Add connections between routers of tiles.
    # The pattern of connection will assume a roughly square tiling.
    # The ceiling of the square root of the number of tiles will be the number of tiles per row.
    # Tiles will be numbered from left to right, top to bottom.
    # The square array may not be full if the number of tiles is not a square.

    for tile in range(0,num_tiles):
        tiles_per_row = math.ceil(math.sqrt(num_tiles))
        # First row
        if (tile < tiles_per_row):
            if tile != 0:
                if ws_configuration == "ws-48":
                    # If the tile is in the first row, connect to the router in the tile to the left.
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Router_" + str(tile-1) + "\"\n\t\tblock1=\"Router_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Router_" + str(tile) + "\"\n\t\tblock1=\"Router_" + str(tile-1) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                elif ws_configuration == "ws-3":
                    # If the tile is in the first row, connect to the network in the tile to the left.
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Network_" + str(tile-1) + "\"\n\t\tblock1=\"Network_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Network_" + str(tile) + "\"\n\t\tblock1=\"Network_" + str(tile-1) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                elif ws_configuration == "ws-1":
                    # If the tile is in the first row, connect to the tile to the left.
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Tile_" + str(tile-1) + "\"\n\t\tblock1=\"Tile_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Tile_" + str(tile) + "\"\n\t\tblock1=\"Tile_" + str(tile-1) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
        # Remaining Rows
        else :
            if ws_configuration == "ws-48":
                # Connect to the tile above
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Router_" + str(tile) + "\"\n\t\tblock1=\"Router_" + str(tile-tiles_per_row) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Router_" + str(tile-tiles_per_row) + "\"\n\t\tblock1=\"Router_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                if (tile % tiles_per_row != 0):
                    # Connect to the previous tile in the row.
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Router_" + str(tile) + "\"\n\t\tblock1=\"Router_" + str(tile-1) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Router_" + str(tile-1) + "\"\n\t\tblock1=\"Router_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
            elif ws_configuration == "ws-3":
                # Connect to the tile above
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Network_" + str(tile) + "\"\n\t\tblock1=\"Network_" + str(tile-tiles_per_row) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Network_" + str(tile-tiles_per_row) + "\"\n\t\tblock1=\"Network_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                if (tile % tiles_per_row != 0):
                    # Connect to the previous tile in the row.
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Network_" + str(tile) + "\"\n\t\tblock1=\"Network_" + str(tile-1) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Network_" + str(tile-1) + "\"\n\t\tblock1=\"Network_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
            elif ws_configuration == "ws-1":
                # Connect to the tile above
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Tile_" + str(tile) + "\"\n\t\tblock1=\"Tile_" + str(tile-tiles_per_row) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Tile_" + str(tile-tiles_per_row) + "\"\n\t\tblock1=\"Tile_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                if (tile % tiles_per_row != 0):
                    # Connect to the previous tile in the row.
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Tile_" + str(tile) + "\"\n\t\tblock1=\"Tile_" + str(tile-1) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
                    netlist.append("\t<net type=\"" + io_cell_name + "\"\n\t\tblock0=\"Tile_" + str(tile-1) + "\"\n\t\tblock1=\"Tile_" + str(tile) + "\"\n\t\tbb_count=\"\"\n\t\tbandwidth=\"" + str(rents_rule_scaling*202) + "\"\n\t\taverage_bandwidth_utilization=\"0.5\">\n\t</net>")
            
    netlist.append("</netlist>")

    # Write the netlist to a file
    with open(block_level_netlist_filename, 'w') as f:
        for line in netlist:
            f.write(line + "\n")

    return block_level_netlist_filename


# Main function
def main():
    # Inputs from the command line:
    #   1. Configuration
    #   2. Number of tiles
    #   3. Number of cores per tile
    #   4. Number of shared memories per tile
    #   5. Area multiplier
    #   6. Power multiplier

    if (len(sys.argv) != 7):
        print("Error: Incorrect number of input arguments.")
        print("Correct Usage: python generateSystemDefinition.py <configuration> <number_of_tiles> <number_of_cores_per_tile> <number_of_shared_memories_per_tile> <area_multiplier> <power_multiplier>")
        print("Valid configurations are " + str(valid_configurations) + ".")
        print("Exiting...")
        exit()

    ws_configuration = sys.argv[1]
    # If ws_configuration is not a valid configuration, exit.
    if ws_configuration not in valid_configurations:
        print("Error: Invalid configuration.")
        print("Valid configurations are " + str(valid_configurations) + ".")
        print("Exiting...")
        exit()
    num_tiles = int(sys.argv[2])
    num_cores_per_tile = int(sys.argv[3])
    num_shared_memories_per_tile = int(sys.argv[4])
    area_multiplier = int(sys.argv[5])
    power_multiplier = int(sys.argv[6])


    block_def_file = generate_block_definitions(ws_configuration,num_tiles,num_cores_per_tile,num_shared_memories_per_tile,area_multiplier,power_multiplier)
    if block_def_file:
        print("Block Definition File Generated: " + block_def_file)

    block_level_netlist_file = generate_block_level_netlist(ws_configuration,num_tiles,num_cores_per_tile,num_shared_memories_per_tile,area_multiplier,power_multiplier)
    if block_level_netlist_file:
        print("Block Level Netlist File Generated: " + block_level_netlist_file)

    return


if __name__ == '__main__':
    main()