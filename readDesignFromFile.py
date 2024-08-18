# =========================================================================
# = This file contains the functions to read the given library xml files. =
# =========================================================================

import design as d
import numpy as np
import xml.etree.ElementTree as ET
import math
import sys

# Note that in if you add new parameters to the XML file, you will need to add them to the class constructors in design.py
#       This means that you will also need to add the initial values to the class declaration in the following functions
#       otherwise there will be a rather confusing error since the static attribute will be set True by default.

# Function to read the wafer process definitions.
def wafer_process_definition_list_from_file(filename):
    # print("Reading wafer process definitions from file: " + filename)
    # Read the XML file.
    tree = ET.parse(filename)
    # Get root of the XML file.
    root = tree.getroot()
    # List of wafer process .
    wp_list = []
    # Iterate over the IO definitions.
    for wp_def in root:
        # Create an Wafer Process object.
        wp = d.WaferProcess(name = "", wafer_diameter = 0.0, edge_exclusion = 0.0, wafer_process_yield = 0.0,
                            dicing_distance = 0.0, reticle_x = 0.0, reticle_y = 0.0, wafer_fill_grid = False,
                            nre_front_end_cost_per_mm2_memory = 0.0, nre_back_end_cost_per_mm2_memory = 0.0,
                            nre_front_end_cost_per_mm2_logic = 0.0, nre_back_end_cost_per_mm2_logic = 0.0,
                            nre_front_end_cost_per_mm2_analog = 0.0, nre_back_end_cost_per_mm2_analog = 0.0,
                            static = False)
        attributes = wp_def.attrib
        # Iterate over the IO definition attributes.
        # Set the IO object attributes.
        wp.set_name(attributes["name"])
        wp.set_wafer_diameter(float(attributes["wafer_diameter"]))
        wp.set_edge_exclusion(float(attributes["edge_exclusion"]))
        wp.set_wafer_process_yield(float(attributes["wafer_process_yield"]))
        wp.set_dicing_distance(float(attributes["dicing_distance"]))
        wp.set_reticle_x(float(attributes["reticle_x"]))
        wp.set_reticle_y(float(attributes["reticle_y"]))
        wp.set_wafer_fill_grid(attributes["wafer_fill_grid"])
        wp.set_nre_front_end_cost_per_mm2_memory(float(attributes["nre_front_end_cost_per_mm2_memory"]))
        wp.set_nre_back_end_cost_per_mm2_memory(float(attributes["nre_back_end_cost_per_mm2_memory"]))
        wp.set_nre_front_end_cost_per_mm2_logic(float(attributes["nre_front_end_cost_per_mm2_logic"]))
        wp.set_nre_back_end_cost_per_mm2_logic(float(attributes["nre_back_end_cost_per_mm2_logic"]))
        wp.set_nre_front_end_cost_per_mm2_analog(float(attributes["nre_front_end_cost_per_mm2_analog"]))
        wp.set_nre_back_end_cost_per_mm2_analog(float(attributes["nre_back_end_cost_per_mm2_analog"]))
        wp.set_static()
        # Append the wafer_process object to the list.
        wp_list.append(wp)
    # print("At end of wafer_process_definition_list_from_file")
    # Return the list of wafer process definition objects.
    return wp_list

# Define a function to read the IO definitions from a file.
def io_definition_list_from_file(filename):
    # print("Reading IO definitions from file: " + filename)
    # Read the XML file.
    tree = ET.parse(filename)
    root = tree.getroot()
    # Create a list of IO objects.
    io_list = []
    # Iterate over the IO definitions.
    for io_def in root:
        # Create an IO object.
        io = d.IO(type = "", rx_area = 0.0, tx_area = 0.0, shoreline = 0.0, bandwidth = 0.0, wire_count = 0,
                  bidirectional = False, energy_per_bit = 0.0, reach = 0.0, static = False)
        attributes = io_def.attrib
        # Iterate over the IO definition attributes.
        # Set the IO object attributes.
        io.set_type(attributes["type"])
        io.set_rx_area(float(attributes["rx_area"]))
        io.set_tx_area(float(attributes["tx_area"]))
        io.set_shoreline(float(attributes["shoreline"]))
        io.set_bandwidth(float(attributes["bandwidth"]))
        io.set_wire_count(int(attributes["wire_count"]))
        io.set_bidirectional(bool(attributes["bidirectional"]))
        io.set_energy_per_bit(float(attributes["energy_per_bit"]))
        io.set_reach(float(attributes["reach"]))
        io.set_static()
        # Append the IO object to the list.
        io_list.append(io)
    # Return the list of IO objects.
    return io_list

# Define a function to read the layer definitions from a file.
def layer_definition_list_from_file(filename):
    # print("Reading layer definitions from file: " + filename)
    # Read the XML file.
    tree = ET.parse(filename)
    root = tree.getroot()
    # Create a list of layer objects.
    layer_list = []
    # Iterate over the layer definitions.
    for layer_def in root:
        # Create a layer object.
        layer = d.Layer(name = "", active = False, cost_per_mm2 = 0, defect_density = 0, critical_area_ratio = 0,
                        clustering_factor = 0, litho_percent = 0, mask_cost = 0, stitching_yield = 0, static = False)
        attributes = layer_def.attrib
        # Set the layer object attributes.
        layer.set_name(attributes["name"])
        layer.set_active(bool(attributes["active"]))
        layer.set_cost_per_mm2(float(attributes["cost_per_mm2"]))
        layer.set_defect_density(float(attributes["defect_density"]))
        layer.set_critical_area_ratio(float(attributes["critical_area_ratio"]))
        layer.set_clustering_factor(float(attributes["clustering_factor"]))
        layer.set_litho_percent(float(attributes["litho_percent"]))
        layer.set_mask_cost(float(attributes["nre_mask_cost"]))
        layer.set_stitching_yield(float(attributes["stitching_yield"]))

        layer.set_static()
        # Append the layer object to the list.
        layer_list.append(layer)
    # Return the list of layer objects.
    # print("At end of layer_list_from_file")
    return layer_list

# Define a function to read the assembly process definitions from a file.
def assembly_process_definition_list_from_file(filename):
    # print("Reading assembly process definitions from file: " + filename)
    # Read the XML file.
    tree = ET.parse(filename)
    root = tree.getroot()
    # Create a list of assembly process objects.
    assembly_process_list = []
    # Iterate over the assembly process definitions.
    for assembly_process_def in root:
        # Create an assembly process object.
        assembly_process = d.Assembly(name = "", materials_cost_per_mm2 = 0.0, picknplace_machine_cost = 0.0,
                                      picknplace_machine_lifetime = 0.0, picknplace_machine_uptime = 0.0, picknplace_technician_yearly_cost = 0.0,
                                      picknplace_time = 0.0, picknplace_group = 0, bonding_machine_cost = 0.0, bonding_machine_lifetime = 0.0,
                                      bonding_machine_uptime = 0.0, bonding_machine_technician_yearly_cost = 0.0, bonding_time = 0.0,
                                      bonding_group = 0, die_separation = 0.0, edge_exclusion = 0.0, bonding_pitch = 0.0, max_pad_current_density = 0.0,
                                      alignment_yield = 0.0, bonding_yield = 0.0, dielectric_bond_defect_density = 0.0, static = False)
        
        attributes = assembly_process_def.attrib

        assembly_process.set_name(attributes["name"])
#        # The following would set machine and technician parameters as a list of an undefined length.
#        # This has been switched to defining these parameters in terms of two values, one for bonding and one for pick and place.
#        # Leaving this for now, in case a reason comes up to rever to the old version.
#        assembly_process.set_machine_cost_list([float(x) for x in attributes["machine_cost_list"].split(',')])
#        assembly_process.set_machine_lifetime_list([float(x) for x in attributes["machine_lifetime_list"].split(',')])
#        assembly_process.set_machine_uptime_list([float(x) for x in attributes["machine_uptime_list"].split(',')])
#        assembly_process.set_technician_yearly_cost_list([float(x) for x in attributes["technician_yearly_cost_list"].split(',')])
        assembly_process.set_materials_cost_per_mm2(float(attributes["materials_cost_per_mm2"]))
        assembly_process.set_picknplace_machine_cost(float(attributes["picknplace_machine_cost"]))
        assembly_process.set_picknplace_machine_lifetime(float(attributes["picknplace_machine_lifetime"]))
        assembly_process.set_picknplace_machine_uptime(float(attributes["picknplace_machine_uptime"]))
        assembly_process.set_picknplace_technician_yearly_cost(float(attributes["picknplace_technician_yearly_cost"]))
        assembly_process.set_picknplace_time(float(attributes["picknplace_time"]))
        assembly_process.set_picknplace_group(int(attributes["picknplace_group"]))
        assembly_process.set_bonding_machine_cost(float(attributes["bonding_machine_cost"]))
        assembly_process.set_bonding_machine_lifetime(float(attributes["bonding_machine_lifetime"]))
        assembly_process.set_bonding_machine_uptime(float(attributes["bonding_machine_uptime"]))
        assembly_process.set_bonding_technician_yearly_cost(float(attributes["bonding_technician_yearly_cost"]))
        assembly_process.set_bonding_time(float(attributes["bonding_time"]))
        assembly_process.set_bonding_group(int(attributes["bonding_group"]))
        assembly_process.compute_picknplace_cost_per_second()
        assembly_process.compute_bonding_cost_per_second()
        assembly_process.set_die_separation(float(attributes["die_separation"]))
        assembly_process.set_edge_exclusion(float(attributes["edge_exclusion"]))
        assembly_process.set_bonding_pitch(float(attributes["bonding_pitch"]))
        assembly_process.set_max_pad_current_density(float(attributes["max_pad_current_density"]))
        assembly_process.set_alignment_yield(float(attributes["alignment_yield"]))
        assembly_process.set_bonding_yield(float(attributes["bonding_yield"]))
        assembly_process.set_dielectric_bond_defect_density(float(attributes["dielectric_bond_defect_density"]))

        assembly_process.set_static()

        # Append the assembly process object to the list.
        assembly_process_list.append(assembly_process)
    # Return the list of assembly process objects.
    return assembly_process_list

# Define a function to read the test process definitions from a file.
def test_process_definition_list_from_file(filename):
    # print("Reading test process definitions from file: " + filename)
    # Read the XML file.
    tree = ET.parse(filename)
    root = tree.getroot()
    # Create a list of test process objects.
    test_process_list = []
    # Iterate over the test process definitions.
    for test_process_def in root:
        # Create a test process object.
        test_process = d.Test(name = "", test_self = False, test_assembly = False, self_defect_coverage = 0.0,
                              assembly_defect_coverage = 0.0, self_test_cost_per_mm2 = 0.0, assembly_test_cost_per_mm2 = 0.0,
                              self_pattern_count = 0, assembly_pattern_count = 0, self_test_failure_dist = 0, assembly_test_failure_dist = 0, static = False)

        attributes = test_process_def.attrib
        test_process.set_name(attributes["name"])
        test_process.set_test_self(True if attributes["test_self"] == "True" or attributes["test_self"] == "TRUE" or attributes["test_self"] == "true" else False)
        test_process.set_test_assembly(True if attributes["test_assembly"] == "True" or attributes["test_assembly"] == "TRUE" or attributes["test_assembly"] == "true" else False)
        test_process.set_self_defect_coverage(float(attributes["self_defect_coverage"]))
        test_process.set_assembly_defect_coverage(float(attributes["assembly_defect_coverage"]))
        test_process.set_self_test_cost_per_mm2(float(attributes["self_test_cost_per_mm2"]))
        test_process.set_assembly_test_cost_per_mm2(float(attributes["assembly_test_cost_per_mm2"]))
        test_process.set_self_pattern_count(int(attributes["self_pattern_count"]))
        test_process.set_assembly_pattern_count(int(attributes["assembly_pattern_count"]))
        test_process.set_self_test_failure_dist(int(attributes["self_test_failure_dist"]))
        test_process.set_assembly_test_failure_dist(int(attributes["assembly_test_failure_dist"]))
        test_process.set_static()

        test_process_list.append(test_process)
    
    return test_process_list

# Define a function to construct the global adjacency matrix from the netlist file.
def global_adjacency_matrix_from_file(filename, io_list):
    # TOSO: Add comments on building adjacency matrix.
    # print("Reading netlist from file: " + filename)
    # Read the XML file.
    tree = ET.parse(filename)
    root = tree.getroot()

    # Split the root.attrib["block_names"] string at commas and store in list.
    block_names = [] 

    # The output format is a dictionary of items with format {type: numpy array adjacencey matrix}
    global_adjacency_matrix = {}
    # The following contains entries mirroring the global adjacency matrix
    # For each entry in the global adjacency matrix, this indicates the average bandwidth utilization
    # that should be used for the power calculation.
    average_bandwidth_utilization = {}

    # Iterate over the net definitions.
    for net_def in root:
        link_average_bandwidth_utilization = float(net_def.attrib["average_bandwidth_utilization"])
        link_type = net_def.attrib["type"]
        # Check if the type of net is already a key in the dictionary.
        if link_type not in global_adjacency_matrix:
            # print("Adding new net type to adjacency matrix: " + link_type)
            # If so, append the new net to the existing adjacency matrix.
            global_adjacency_matrix[link_type] = np.zeros((len(block_names), len(block_names)))
            average_bandwidth_utilization[link_type] = np.zeros((len(block_names), len(block_names)))

        # If block 0 or block 1 is not in the list of block names, add it.
        if net_def.attrib["block0"] not in block_names:
            # print("Adding new block to adjacency matrix: " + net_def.attrib["block0"])
            block_names.append(net_def.attrib["block0"])
            # Add a row and column to each numpy adjacency matrix.
            for key in global_adjacency_matrix:
                global_adjacency_matrix[key] = np.pad(global_adjacency_matrix[key], ((0,1),(0,1)), 'constant', constant_values=0)
                average_bandwidth_utilization[key] = np.pad(average_bandwidth_utilization[key], ((0,1),(0,1)), 'constant', constant_values=0)
        if net_def.attrib["block1"] not in block_names:
            # print("Adding new block to adjacency matrix: " + net_def.attrib["block1"])
            block_names.append(net_def.attrib["block1"])
            # Add a row and column to each numpy adjacency matrix.
            for key in global_adjacency_matrix:
                global_adjacency_matrix[key] = np.pad(global_adjacency_matrix[key], ((0,1),(0,1)), 'constant', constant_values=0)
                average_bandwidth_utilization[key] = np.pad(average_bandwidth_utilization[key], ((0,1),(0,1)), 'constant', constant_values=0)

        io_bandwidth = None
        bidirectional = None   
        
        # print("Searching for and IO type that matches " + link_type)
        for io in io_list:
            #print(io.get_type())
            if link_type == io.get_type():
                io_bandwidth = io.get_bandwidth()
                bidirectional = io.get_bidirectional()

        if io_bandwidth is None or bidirectional is None:
            print("ERROR: Net type " + link_type + " not found in io_list.")
            sys.exit(1)

        # Find the indices of the two blocks connected by the net.
        block1_index = block_names.index(net_def.attrib["block0"])
        block2_index = block_names.index(net_def.attrib["block1"])

        if net_def.attrib["bb_count"] == "":
            ios_to_add = int(math.ceil(float(net_def.attrib["bandwidth"])/io_bandwidth))
        else:
            ios_to_add = int(net_def.attrib["bb_count"])
        peak_utilization_factor = float(net_def.attrib["bandwidth"])/(ios_to_add*io_bandwidth)
        link_average_bandwidth_utilization *= peak_utilization_factor
        if not bidirectional:
            if average_bandwidth_utilization[link_type][block1_index,block2_index] == 0:
                average_bandwidth_utilization[link_type][block1_index,block2_index] = link_average_bandwidth_utilization
            else:
                average_bandwidth_utilization[link_type][block1_index,block2_index] = (average_bandwidth_utilization[link_type][block1_index,block2_index]*global_adjacency_matrix[link_type][block1_index,block2_index] + link_average_bandwidth_utilization*ios_to_add)/(global_adjacency_matrix[link_type][block1_index,block2_index] + ios_to_add)
            global_adjacency_matrix[link_type][block1_index,block2_index] += ios_to_add 
        else:
            if average_bandwidth_utilization[link_type][block1_index,block2_index] == 0:
                average_bandwidth_utilization[link_type][block1_index,block2_index] = link_average_bandwidth_utilization
            else:
                average_bandwidth_utilization[link_type][block1_index,block2_index] = (average_bandwidth_utilization[link_type][block1_index,block2_index]*global_adjacency_matrix[link_type][block1_index,block2_index] + link_average_bandwidth_utilization*ios_to_add)/(global_adjacency_matrix[link_type][block1_index,block2_index] + ios_to_add)
            global_adjacency_matrix[link_type][block1_index,block2_index] += ios_to_add
            if average_bandwidth_utilization[link_type][block2_index,block1_index] == 0:
                average_bandwidth_utilization[link_type][block2_index,block1_index] = link_average_bandwidth_utilization
            else:
                average_bandwidth_utilization[link_type][block2_index,block1_index] = (average_bandwidth_utilization[link_type][block2_index,block1_index]*global_adjacency_matrix[link_type][block2_index,block1_index] + link_average_bandwidth_utilization*ios_to_add)/(global_adjacency_matrix[link_type][block2_index,block1_index] + ios_to_add)
            global_adjacency_matrix[link_type][block2_index,block1_index] += ios_to_add

    return global_adjacency_matrix, average_bandwidth_utilization, block_names

# Define a function to construct the root chip and all subchips from a definition file.
def chip_from_dict(etree, io_list, layer_list, wafer_process_list, assembly_process_list, test_process_list, global_adjacency_matrix, average_bandwidth_utilization, block_names):
    chip = d.Chip(filename = None, etree = etree, parent_chip = None, wafer_process_list = wafer_process_list, assembly_process_list = assembly_process_list, test_process_list = test_process_list, layers = layer_list, ios = io_list, adjacency_matrix_definitions = global_adjacency_matrix, average_bandwidth_utilization=average_bandwidth_utilization, block_names = block_names, static = False)
    return chip
