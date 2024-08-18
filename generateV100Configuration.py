import numpy as np
import sys
import math

# Physical Definition File Function
def generate_physical_definition_file(num_v100s, interposer_assembly_process):

    filename = "v100_" + str(num_v100s) + ".xml"

    file_header = ("<!--\n\tFilename: " + filename + "\n\tAuthor: Alexander Graening" 
                    + "\n\tAffiliation: University of California, Los Angeles"
                    + "\n\tEmail: agraening@ucla.edu\n\n\tSystem Definition Format:"
                    + "\n\t\t- Black Box Parameters (Area, Cost, Quality, Power): These are the"
                    + "\n\t\t\tblack box parameters for the chip. These can be used to override"
                    + "\n\t\t\tthe computed parameters. For normal operation of the cost model to"
                    + "\n\t\t\tcompute these parameters, set these to the empty string \"\". The"
                    + "\n\t\t\tarea is in mm^2, the cost is in USD, the quality is a number between"
                    + "\n\t\t\t0.0 and 1.0, and the power is in Watts."
                    + "\n\t\t- Core Area: This is the area required for logic and memory. A passive"
                    + "\n\t\t\tintegration substrate would have a core area of 0 since the real"
                    + "\n\t\t\tarea is determined by the logic area of the dies stacked on it."
                    + "\n\t\t- Fraction Memory: This is the fraction of the core area that is memory."
                    + "\n\t\t\tThis is used for NRE design costs. Valid values are between 0.0 and 1.0."
                    + "\n\t\t- Fraction Logic: This is the fraction of the core area that is logic."
                    + "\n\t\t\tThis is used for NRE design costs. Valid values are between 0.0 and 1.0."
                    + "\n\t\t- Fraction Analog: This is the fraction of the core area that is analog."    
                    + "\n\t\t\tThis is used for NRE design costs. Valid values are between 0.0 and 1.0."
                    + "\n\t\t- Reticle Share: Since it is possible to share mask costs by splitting"
                    + "\n\t\t\tup among multiple designs, this parameter can be used to give the"
                    + "\n\t\t\tfraction of a shared reticle that is used by this design. Note that"
                    + "\n\t\t\teven if there is not a perfect reticle fit, the reticle share of 1.0"
                    + "\n\t\t\tshould be used to indicate that there is not sharing. Valid values"
                    + "\n\t\t\tare between 0.0 and 1.0."
                    + "\n\t\t- Buried: If this flag is true, a die does not affect the area of"   
                    + "\n\t\t\tthe integration substrate it is stacked on. This is useful for"
                    + "\n\t\t\tmodelling EMIB."
                    + "\n\t\t- Assembly Process: An assembly process should be selected from the"
                    + "\n\t\t\tassembly process file here."
                    + "\n\t\t- Test Process: Select a test process from the test process file here."
                    + "\n\t\t\tNote that this is still under development, but an update will come"
                    + "\n\t\t\tsupport."
                    + "\n\t\t- Stackup: The stackup is defined as a comma separated list of layers"
                    + "\n\t\t\twhere each later starts with a number indicating how many times the"
                    + "\n\t\t\tlayer is repeated. The layer name is then given after the colon."
                    + "\n\t\t\tFor example: \"1:active_layer,2:advanced_metal_layer,2:intermediate_metal_layer,"
                    + "\n\t\t\t2:global_metal_layer\" It is also valid to simply define a single layer."
                    + "\n\t\t- Wafer Process: Select a wafer process from the wafer process file here."
                    + "\n\t\t- NRE Design Cost: This is the non-recurring engineering cost for the chip."
                    + "\n\t\t\tWe plan to add more details to the NRE cost modelling support."
                    + "\n\t\t- Core Voltage: This is the core voltage for the chip used to determine"
                    + "\n\t\t\tcurrent density based on power number."
                    + "\n\t\t- Power: This is the power of the chip used to determine the number of"
                    + "\n\t\t\tVDD/GND bumps along with the core voltage."
                    + "\n\t\t- Quantity: This determines the ammortization of the NRE costs."
                    + "\n\t=== The next few parameters will be used in the PDN definition but are not currently supported. ==="
                    + "\n\t\t- V Rail: This is the voltage rail for the chip. If there are multiple"
                    + "\n\t\t\tvoltage rails, they should be comma separated."
                    + "\n\t\t- Regulator Efficiency: This is the efficiency of the voltage regulator"
                    + "\n\t\t\tfor the chip. If there are multiple regulators, they should be comma"
                    + "\n\t\t\tseparated."
                    + "\n\t\t- Regulator Type: This is the type of voltage regulator for the chip."
                    + "\n\t\t\tIf there are multiple regulators, they should be comma separated. This is mean to"
                    + "\n\t\t\t"
                    + "\n\t\t- Defining 2.5D and 3D stacks:"
                    + "\n\t\t\t- As can be seen in the design below, each chip has may contain a list of nested"
                    + "\n\t\t\t\tchip objects."
                    + "\n\t\t\t- The root chip is considered to be on the bottom and each chip in its list is"
                    + "\n\t\t\t\tstacked directly on top of it."
                    + "\n\t\t\t- Each of those stacked chips may also contain a list of nested chip objects"
                    + "\n\t\t\t\twhich are stacked directly on it."
                    + "\n\tDescription: Model for NVIDIA V100. This consists a grid of units composed of one GPU chip surrounded by 4 HBM chips. There are " + str(num_v100s) + " units connected in a grid."
                    + "\n\n\tNote that each parameter is required to be set even if it is not relevant to the application."
                    + "\n\t\tIf is it not used, select a relevant value that will interact correctly with the cost model."
                    + "\n\t\tIf no value is selected, at least define the line as the empty string: <parameter_name=\"\">."
                    + "\n\t\tA list of required parameters for the chip object is below:"
                    + "\n\t\t\t- name"
                    + "\n\t\t\t- bb_area"
                    + "\n\t\t\t- bb_cost"
                    + "\n\t\t\t- bb_quality"
                    + "\n\t\t\t- bb_power"
                    + "\n\t\t\t- coreArea"
                    + "\n\t\t\t- fractionMemory"
                    + "\n\t\t\t- fractionLogic"
                    + "\n\t\t\t- fractionAnalog"
                    + "\n\t\t\t- reticleShare"
                    + "\n\t\t\t- buried"
                    + "\n\t\t\t- assembly_process"
                    + "\n\t\t\t- test_process"
                    + "\n\t\t\t- stackup"
                    + "\n\t\t\t- wafer_process"
                    + "\n\t\t\t- nre_design_cost"
                    + "\n\t\t\t- core_voltage"
                    + "\n\t\t\t- power"
                    + "\n\t\t\t- quantity"
                    + "\n-->\n\n")

    file_footer = "\n</chip>\n"
    
    content = ("<chip name=\"interposer\""
                + "\n\tbb_area=\"\""
                + "\n\tbb_cost=\"\""
                + "\n\tbb_quality=\"\""
                + "\n\tbb_power=\"\""
                + "\n\n\tcore_area=\"0.0\""
                + "\n\tfraction_memory=\"0.0\""
                + "\n\tfraction_logic=\"0.0\""
                + "\n\tfraction_analog=\"0.0\""
                + "\n\treticle_share=\"1.0\""
                + "\n\tburied=\"False\""
                + "\n\tassembly_process=\"" + interposer_assembly_process + "\""
                + "\n\ttest_process=\"KGD_free_test\""
                + "\n\tstackup=\"1:organic_substrate\""
                + "\n\twafer_process=\"process_1\""
                + "\n\tcore_voltage=\"1.8\""
                + "\n\tpower=\"0.0\""
                + "\n\tquantity=\"1000000\""
                + "\n\tv_rail=\"\""
                + "\n\treg_eff=\"\""
                + "\n\treg_type=\"\">")

    for i in range(num_v100s):
        v100_def = ""

        gpu_def = ("\n\t<chip name=\"gpu_" + str(i) + "\""
                   + "\n\t\tbb_area=\"\""
                   + "\n\t\tbb_cost=\"\""
                   + "\n\t\tbb_quality=\"\""
                   + "\n\t\tbb_power=\"200.0\""
                   + "\n\n\t\tcore_area=\"815.0\""
                   + "\n\t\tfraction_memory=\"0.0\""
                   + "\n\t\tfraction_logic=\"0.0\""
                   + "\n\t\tfraction_analog=\"0.0\""
                   + "\n\t\treticle_share=\"1.0\""
                   + "\n\t\tburied=\"False\""
                   + "\n\t\tassembly_process=\"" + interposer_assembly_process + "\""
                   + "\n\t\ttest_process=\"self_test_only_KGD_free_test\""
                   + "\n\t\tstackup=\"1:combined_12nm\""
                   + "\n\t\twafer_process=\"process_1\""
                   + "\n\t\tcore_voltage=\"1.8\""
                   + "\n\t\tpower=\"200.0\""
                   + "\n\t\tquantity=\"1000000\""
                   + "\n\t\tv_rail=\"\""
                   + "\n\t\treg_eff=\"\""
                   + "\n\t\treg_type=\"\">\n\t</chip>")

        v100_def += gpu_def

        for j in range(4):
            hbm_def = ("\n\t<chip name=\"hbm_" + str(j) + "_" + str(i) + "\""
                       + "\n\t\tbb_area=\"80\""
                       + "\n\t\tbb_cost=\"\""
                       + "\n\t\tbb_quality=\"\""
                       + "\n\t\tbb_power=\"25.0\""
                       + "\n\n\t\tcore_area=\"0.0\""
                       + "\n\t\tfraction_memory=\"1.0\""
                       + "\n\t\tfraction_logic=\"0.0\""
                       + "\n\t\tfraction_analog=\"0.0\""
                       + "\n\t\treticle_share=\"1.0\""
                       + "\n\t\tburied=\"False\""
                       + "\n\t\tassembly_process=\"" + interposer_assembly_process + "\""
                       + "\n\t\ttest_process=\"self_test_only_KGD_free_test\""
                       + "\n\t\tstackup=\"1:combined_hbm2_12nm\""
                       + "\n\t\twafer_process=\"process_1\""
                       + "\n\t\tcore_voltage=\"1.8\""
                       + "\n\t\tpower=\"0\""
                       + "\n\t\tquantity=\"4000000\""
                       + "\n\t\tv_rail=\"\""
                       + "\n\t\treg_eff=\"\""
                       + "\n\t\treg_type=\"\">\n\t</chip>")

            v100_def += hbm_def

        content += v100_def

    # Create a file with the filename above and write the header to the file.
    with open(filename, 'w') as f:
        f.write(file_header)
        f.write(content)
        f.write(file_footer)

    return filename

# Netlist File Function
def generate_netlist(num_v100s, inter_node_bandwidth, inter_package_bandwidth, io_type):

    filename = "v100_netlist_" + str(num_v100s) + ".xml"

    file_header = ("<!--\n\tFilename: " + filename + "\n\tAuthor: Alexander Graening"
                   + "\n\tAffiliation: University of California, Los Angeles"
                   + "\n\tEmail: agraening@ucla.edu\n"
                   + "\n\tNetlist Format:"
                   + "\n\t\t- IO Definition Reference: IO type is defined with the type parameter."
                   + "\n\t\t\tThis name must reference an IO type in the io definition file."
                   + "\n\t\t- Directionality: If the IO type referenced from the io definition"
                   + "\n\t\t\tfile is defined as a bidirectional IO, there is no sense of direction."
                   + "\n\t\t\tAlternatively, if the IO is not defined as bidirectional, the"
                   + "\n\t\t\tconnections are directional with block0 as the TX block and block1"
                   + "\n\t\t\tas the RX block."
                   + "\n\t\t- Area for Bidirectional Cells: If there is a bidirectional IO type"
                   + "\n\t\t\tbetween blocks in different technology nodes it is possible the"
                   + "\n\t\t\tTX and RX sides will have different areas despite being functionally"
                   + "\n\t\t\tequivalent. In this case, the area of the bidirectional IO follows"
                   + "\n\t\t\tthe same format as above, block0 will have the area defined as TX"
                   + "\n\t\t\tarea in the IO definition."
                   + "\n\t\t- Black Box Count: This is the number of instances of the IO. This will"
                   + "\n\t\t\toverride the bandwidth calculation, so ignore leave this as the empty"
                   + "\n\t\t\tstring if you want the number of instances to be calculated from the"
                   + "\n\t\t\tbandwidth and IO definition."
                   + "\n\t\t- Bandwidth: The bandwidth is defined in G-bit/s. This is the bandwidth"
                   + "\n\t\t\tof the connection in one direction for a directional connection. If"
                   + "\n\t\t\tthe IO is bidirectional, this is the combined bandwidth in both"
                   + "\n\t\t\tdirections. This is assumed to be symmetric so 10G-bit/s would mean"
                   + "\n\t\t\t5G-bit/s in each direction."
                   + "\n\t\t- Average Bandwidth Utilization: This is the average bandwidth utilization"
                   + "\n\t\t\tof the connection. This is used for the power calculation and should be"
                   + "\n\t\t\ta number between 0 and 1.\n"
                   + "\n\tDescription: Connectivity for v100 sample design with " + str(num_v100s) + " v100s."
                   + "\n\n\tNote that each parameter is required to be set to avoid throwing an error. Either"
                   + "\n\t\tthe bb_count or the bandwidth must be defined. If bb_count is defined, the"
                   + "\n\t\tvalue set for bandwidth does not matter. To use the bandwidth calculation,"
                   + "\n\t\tset bb_count=\"\"."
                   + "\n\t\tA list of required parameters is below:"
                   + "\n\t\t\t- type"
                   + "\n\t\t\t- block0"
                   + "\n\t\t\t- block1"
                   + "\n\t\t\t- bb_count"
                   + "\n\t\t\t- bandwidth"
                   + "\n\t\t\t- average_bandwidth_utilization"
                   + "\n-->\n\n"
                   + "<netlist>\n")
    
    file_footer = "\n</netlist>\n"

    content = ""

    # Generate the internal V100 connections.
    for i in range(num_v100s):
        # GPU to HBM connections
        for j in range(4):
            content += ("\n\t<net type=\"UCIe_standard\""
                        + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                        + "\n\t\tblock1=\"hbm_" + str(j) + "_" + str(i) + "\""
                        + "\n\t\tbb_count=\"\""
                        + "\n\t\tbandwidth=\"3104.0\""
                        + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                        + "\n\t</net>")

#    # Generate the internal V100 connections.
#    for i in range(num_v100s):
#        # GPU to HBM connections
#        for j in range(4):
#            content += ("\n\t<net type=\"2Gbs_100vCDM_2mm\""
#                        + "\n\t\tblock0=\"gpu_" + str(i) + "\""
#                        + "\n\t\tblock1=\"hbm_" + str(j) + "_" + str(i) + "\""
#                        + "\n\t\tbb_count=\"\""
#                        + "\n\t\tbandwidth=\"1552.0\""
#                        + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
#                        + "\n\t</net>")
#            content += ("\n\t<net type=\"2Gbs_100vCDM_2mm\""
#                        + "\n\t\tblock0=\"hbm_" + str(j) + "_" + str(i) + "\""
#                        + "\n\t\tblock1=\"gpu_" + str(i) + "\""
#                        + "\n\t\tbb_count=\"\""
#                        + "\n\t\tbandwidth=\"1552.0\""
#                        + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
#                        + "\n\t</net>")

#    # Generate the internal V100 connections.
#    for i in range(num_v100s):
#        # GPU to HBM connections
#        for j in range(4):
#            content += ("\n\t<net type=\"2Gbs_100vCDM_2mm\""
#                        + "\n\t\tblock0=\"gpu_" + str(i) + "\""
#                        + "\n\t\tblock1=\"hbm_" + str(j) + "_" + str(i) + "\""
#                        + "\n\t\tbb_count=\"776\""
#                        + "\n\t\tbandwidth=\"100.0\""
#                        + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
#                        + "\n\t</net>")
#            content += ("\n\t<net type=\"2Gbs_100vCDM_2mm\""
#                        + "\n\t\tblock0=\"hbm_" + str(j) + "_" + str(i) + "\""
#                        + "\n\t\tblock1=\"gpu_" + str(i) + "\""
#                        + "\n\t\tbb_count=\"776\""
#                        + "\n\t\tbandwidth=\"100.0\""
#                        + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
#                        + "\n\t</net>")

    # Generate inter-V100 connections
    # V100s are connected in a square pattern based on the next perfect square larger than the number of V100s
    num_per_side = math.ceil(math.sqrt(num_v100s))
    for i in range(num_v100s):
        # Check if the V100 is on the right edge of the square
        if ((i + 1) % num_per_side != 0):
            # Connect to the V100 to the right if it exists.
            if (i + 1 < num_v100s):
                # if io_type contains "UCIe", then the connections are bidirectional
                if "UCIe" in io_type:
                    content += ("\n\t<net type=\"" + io_type + "\""
                                + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                                + "\n\t\tblock1=\"gpu_" + str(i + 1) + "\""
                                + "\n\t\tbb_count=\"\""
                                + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                                + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                                + "\n\t</net>")
                else:
                    content += ("\n\t<net type=\"" + io_type + "\""
                                + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                                + "\n\t\tblock1=\"gpu_" + str(i + 1) + "\""
                                + "\n\t\tbb_count=\"\""
                                + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                                + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                                + "\n\t</net>")
                    content += ("\n\t<net type=\"" + io_type + "\""
                                + "\n\t\tblock0=\"gpu_" + str(i + 1) + "\""
                                + "\n\t\tblock1=\"gpu_" + str(i) + "\""
                                + "\n\t\tbb_count=\"\""
                                + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                                + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                                + "\n\t</net>")

        # If the V100 is on the right edge, add a wrap-around connection to the V100 on the left.
        if ((i + 1) % num_per_side == 0):
            if "UCIe" in io_type:
                content += ("\n\t<net type=\"" + io_type + "\""
                            + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                            + "\n\t\tblock1=\"gpu_" + str(i + 1 - num_per_side) + "\""
                            + "\n\t\tbb_count=\"\""
                            + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                            + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                            + "\n\t</net>")
            else:
                content += ("\n\t<net type=\"" + io_type + "\""
                            + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                            + "\n\t\tblock1=\"gpu_" + str(i + 1 - num_per_side) + "\""
                            + "\n\t\tbb_count=\"\""
                            + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                            + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                            + "\n\t</net>")
                content += ("\n\t<net type=\"" + io_type + "\""
                            + "\n\t\tblock0=\"gpu_" + str(i + 1 - num_per_side) + "\""
                            + "\n\t\tblock1=\"gpu_" + str(i) + "\""
                            + "\n\t\tbb_count=\"\""
                            + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                            + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                            + "\n\t</net>")

        # Check if the V100 is on the bottom edge of the square
        if (i < num_v100s - num_per_side):
            # Connect to the V100 below if it exists.
            if (i + num_per_side < num_v100s):
                if "UCIe" in io_type:
                    content += ("\n\t<net type=\"" + io_type + "\""
                                + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                                + "\n\t\tblock1=\"gpu_" + str(i + num_per_side) + "\""
                                + "\n\t\tbb_count=\"\""
                                + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                                + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                                + "\n\t</net>")
                else:
                    content += ("\n\t<net type=\"" + io_type + "\""
                                + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                                + "\n\t\tblock1=\"gpu_" + str(i + num_per_side) + "\""
                                + "\n\t\tbb_count=\"\""
                                + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                                + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                                + "\n\t</net>")
                    content += ("\n\t<net type=\"" + io_type + "\""
                                + "\n\t\tblock0=\"gpu_" + str(i + num_per_side) + "\""
                                + "\n\t\tblock1=\"gpu_" + str(i) + "\""
                                + "\n\t\tbb_count=\"\""
                                + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                                + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                                + "\n\t</net>")

        # If the V100 is on the bottom edge, add a wrap-around connection to the V100 on the top.
        if (i >= num_v100s - num_per_side):
            if "UCIe" in io_type:
                content += ("\n\t<net type=\"" + io_type + "\""
                            + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                            + "\n\t\tblock1=\"gpu_" + str(i - num_v100s + num_per_side) + "\""
                            + "\n\t\tbb_count=\"\""
                            + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                            + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                            + "\n\t</net>")
            else:
                content += ("\n\t<net type=\"" + io_type + "\""
                            + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                            + "\n\t\tblock1=\"gpu_" + str(i - num_v100s + num_per_side) + "\""
                            + "\n\t\tbb_count=\"\""
                            + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                            + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                            + "\n\t</net>")
                content += ("\n\t<net type=\"" + io_type + "\""
                            + "\n\t\tblock0=\"gpu_" + str(i - num_v100s + num_per_side) + "\""
                            + "\n\t\tblock1=\"gpu_" + str(i) + "\""
                            + "\n\t\tbb_count=\"\""
                            + "\n\t\tbandwidth=\"" + str(inter_node_bandwidth) + "\""
                            + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                            + "\n\t</net>")
        
        # Add two connections for each GPU to external with inter-package bandwidth
        if "UCIe" in io_type:
            content += ("\n\t<net type=\"" + io_type + "\""
                        + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                        + "\n\t\tblock1=\"external\""
                        + "\n\t\tbb_count=\"\""
                        + "\n\t\tbandwidth=\"" + str(2*inter_package_bandwidth) + "\""
                        + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                        + "\n\t</net>")
        else:
            content += ("\n\t<net type=\"" + io_type + "\""
                        + "\n\t\tblock0=\"gpu_" + str(i) + "\""
                        + "\n\t\tblock1=\"external\""
                        + "\n\t\tbb_count=\"\""
                        + "\n\t\tbandwidth=\"" + str(2*inter_package_bandwidth) + "\""
                        + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                        + "\n\t</net>")
            content += ("\n\t<net type=\"" + io_type + "\""
                        + "\n\t\tblock0=\"external\""
                        + "\n\t\tblock1=\"gpu_" + str(i) + "\""
                        + "\n\t\tbb_count=\"\""
                        + "\n\t\tbandwidth=\"" + str(2*inter_package_bandwidth) + "\""
                        + "\n\t\taverage_bandwidth_utilization=\"0.5\">"
                        + "\n\t</net>")

    # Create a file with the filename above and write the header to the file.
    with open(filename, 'w') as f:
        f.write(file_header)
        f.write(content)
        f.write(file_footer)

    return filename

# Main function
def main():
    # Inputs from the command line:
    #   1. Number of V100s
    # If there is a square number of V100s, this will place in a square configuration.
    # If not, they will be filled in from the top left to the bottom right in the floorplan of the perfect square larger.

    if (len(sys.argv) != 6):
        print("Error: Incorrect number of input arguments.")
        print("Correct Usage: python generateV100Configuration.py <number_of_v100s> <inter_node_bandwidth> <inter_package_bandwidth> <organic/silicon> <UCIe_standard/UCIe_advanced/AIB>")
        print("Exiting...")
        exit()

    num_v100s = int(sys.argv[1])
    inter_node_bandwidth = float(sys.argv[2])
    inter_package_bandwidth = float(sys.argv[3])
    if (sys.argv[4] == "organic"):
        interposer_assembly_process = "organic_simultaneous_bonding"
    elif (sys.argv[4] == "organic_55"):
        interposer_assembly_process = "organic_55_simultaneous_bonding"
    elif (sys.argv[4] == "silicon"):
        interposer_assembly_process = "silicon_individual_bonding"
    elif (sys.argv[4] == "silicon_45"):
        interposer_assembly_process = "silicon_45_individual_bonding"
    else:
        print("Error: Invalid interposer assembly process.")
        print("Exiting...")
        exit()

    if (sys.argv[5] == "UCIe_standard"):
        io_type = "UCIe_standard"
    elif (sys.argv[5] == "UCIe_advanced"):
        io_type = "UCIe_advanced"
    elif (sys.argv[5] == "AIB"):
        io_type = "2Gbs_100vCDM_2mm"
    else:
        print("Error: Invalid IO type.")
        print("Exiting...")
        exit()

    phys_def_file = generate_physical_definition_file(num_v100s, interposer_assembly_process)
    if phys_def_file:
        print("Physical Definition File Generated: " + phys_def_file)

    netlist_file = generate_netlist(num_v100s, inter_node_bandwidth, inter_package_bandwidth, io_type)
    if netlist_file:
        print("Block Level Netlist File Generated: " + netlist_file)

    return


if __name__ == '__main__':
    main()