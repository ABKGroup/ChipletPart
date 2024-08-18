import xml.etree.ElementTree as ET
import numpy as np

tech_nodes = ["45nm", "10nm", "7nm"]
stackup_names = ["1:combined_45nm", "1:combined_10nm", "1:combined_7nm"]

def create_element_tree(subtree_tech_nodes, subtree_aspect_ratios, subtree_x_locations, subtree_y_locations, subtree_power, subtree_coreArea, num_subtrees):
    # print("Number of subtrees: ", num_subtrees)
    # Create the root element with attributes
    root = ET.Element("chip")
    root.set("name", "interposer")
    root.set("bb_area", "")
    root.set("bb_cost", "")
    root.set("bb_power", "")
    root.set("bb_quality", "")
    root.set("aspect_ratio", "")
    root.set("x_location", "")
    root.set("y_location", "")
    root.set("fraction_memory", "0.0")
    root.set("fraction_logic", "0.0")
    root.set("fraction_analog", "1.0")
    root.set("reticle_share", "1.0")
    root.set("buried", "False")
    root.set("assembly_process", "organic_simultaneous_bonding")
    root.set("test_process", "test_process_0")
    root.set("stackup", "1:organic_substrate")
    root.set("wafer_process", "process_1")
    root.set("v_rail", "5")
    root.set("reg_eff", "1.0")
    root.set("reg_type", "none")
    root.set("core_voltage", "1.0")
    root.set("quantity", "1000000")
    root.set("core_area", "0.0")  # Set the "coreArea" attribute for the root
    root.set("power", "0.0")      # Set the "power" attribute for the root

    #print()
    #print("subtree coreArea: ", subtree_coreArea)
    #print()

    # Create the specified number of subtrees with the same attributes
    for i in range(num_subtrees):
        subtree = ET.SubElement(root, "chip")
        subtree.set("name", str(i))
        subtree.set("bb_area", "")
        subtree.set("bb_cost", "")
        subtree.set("bb_power", "")
        subtree.set("bb_quality", "")
        if subtree_aspect_ratios[i] != None:
            subtree.set("aspect_ratio", str(subtree_aspect_ratios[i]))
        else:
            subtree.set("aspect_ratio", "")
        if subtree_x_locations[i] != None:
            subtree.set("x_location", str(subtree_x_locations[i]))
        else:
            subtree.set("x_location", "")
        if subtree_y_locations[i] != None:
            subtree.set("y_location", str(subtree_y_locations[i]))
        else:
            subtree.set("y_location", "")
        subtree.set("fraction_memory", "0.0")
        subtree.set("fraction_logic", "1.0")
        subtree.set("fraction_analog", "0.0")
        subtree.set("reticle_share", "1.0")
        subtree.set("buried", "False")
        subtree.set("assembly_process", "organic_simultaneous_bonding")
        subtree.set("test_process", "test_process_0")
        # Find subtree_tech_nodes[i] in tech_nodes and set the "stackup" attribute accordingly
        index = tech_nodes.index(subtree_tech_nodes[i])
        stackup = stackup_names[index]
        subtree.set("stackup", stackup)
        #if subtree_tech_nodes[i] == "45nm":
        #    subtree.set("stackup", "1:combined_45nm")
        #elif subtree_tech_nodes[i] == "40nm":
        #    subtree.set("stackup", "1:combined_40nm")
        #elif subtree_tech_nodes[i] == "12nm":
        #    subtree.set("stackup", "1:combined_12nm")
        #elif subtree_tech_nodes[i] == "10nm":
        #    subtree.set("stackup", "1:combined_10nm")
        #elif subtree_tech_nodes[i] == "7nm":
        #    subtree.set("stackup", "1:combined_7nm")
        #elif subtree_tech_nodes[i] == "5nm":
        #    subtree.set("stackup", "1:combined_5nm")
        #elif subtree_tech_nodes[i] == "3nm":
        #    subtree.set("stackup", "1:combined_3nm")
        subtree.set("wafer_process", "process_1")
        subtree.set("v_rail", "5")
        subtree.set("reg_eff", "1.0")
        subtree.set("reg_type", "none")
        subtree.set("core_voltage", "1.0")
        subtree.set("quantity", "1000000")

        # Set "coreArea" and "power" attributes for the subtrees from the input lists
        subtree.set("core_area", str(subtree_coreArea[i]))
        # print("Area = " + str(subtree_coreArea[i]))
        subtree.set("power", str(subtree_power[i]))
        # print("Power = " + str(subtree_power[i]))

    # Create the ElementTree
    tree = ET.ElementTree(root)

    # # Print the ElementTree as XML for debugging
    # tree.write("debug_element_tree.xml")

    # # Wait for user input to continue.
    # input("Press Enter to continue...")

    # print(tree.getroot().attrib)

    return tree.getroot()  # Return the ElementTree

def combine_blocks(global_adjacency_matrix, average_bandwidth_utilization, block_names, block_combinations):
    # Create a dictionary to store the adjacency matrices for the combined blocks.
    combined_adjacency_matrix = {}

    # print(global_adjacency_matrix)
    # print(block_names)
    # print(block_combinations)

    combined_adjacency_matrix = {}
    combined_average_bandwidth_utilization = {}

    # block_names = []
    # for partition in range(len(block_combinations)):
        # block_names.append(str(partition))

    # print(block_names)

    # Create a combined adjacency matrix for the new block.
    for io_type, adjacency_matrix in global_adjacency_matrix.items():
        combined_matrix = np.ones((len(block_combinations), len(block_combinations)))
        combined_ave_bw = np.ones((len(block_combinations), len(block_combinations)))
        partition_number = 0
        for partition in block_combinations:
            for block in partition:
                for block_name in block_names:
                    if block_name not in partition:
                        # Find which partition int(block_name) is in
                        block_number = int(block_name)
                        other_partition_number = 0
                        for other_partition in block_combinations:
                            if block_number in other_partition:
                                if combined_matrix[partition_number,other_partition_number] == 0:
                                    combined_ave_bw[partition_number,other_partition_number] = average_bandwidth_utilization[io_type][block, block_number]
                                    combined_matrix[partition_number,other_partition_number] = adjacency_matrix[block, block_number]
                                else:
                                    old_ave_bw_scaled = combined_ave_bw[partition_number,other_partition_number]*combined_matrix[partition_number,other_partition_number] 
                                    new_ave_bw_scaled = average_bandwidth_utilization[io_type][block, block_number]*adjacency_matrix[block, block_number]
                                    combined_ave_bw[partition_number,other_partition_number] = (old_ave_bw_scaled + new_ave_bw_scaled)
                                    combined_matrix[partition_number,other_partition_number] += adjacency_matrix[block, block_number]
                                    combined_ave_bw[partition_number,other_partition_number] /= combined_matrix[partition_number,other_partition_number]
                            other_partition_number += 1
            partition_number += 1

        combined_adjacency_matrix[io_type] = combined_matrix
        combined_average_bandwidth_utilization[io_type] = combined_ave_bw

    return combined_adjacency_matrix, combined_average_bandwidth_utilization, block_names


if __name__ == "__main__":
    # Example values for "power" and "coreArea" lists
    power_values = [1, 2, 3, 4, 5]
    coreArea_values = [10, 20, 30, 40, 50]
    num_subtrees = 5  # Default number of subtrees

    element_tree = create_element_tree(power_values, coreArea_values, num_subtrees)
