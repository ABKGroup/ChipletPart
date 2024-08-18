import math
import sys
import os
# Generate a system definition .xml file.


# As input, take the number of chiplets, the interchiplet IO type, organic/silicon interposer, the external bandwidth, and the power.
if len(sys.argv) != 8:
    print("ERROR: Incorrect Number of Input Arguments\nUsage: python generate_test_files.py <number_of_chiplets> <interchiplet_IO_type> <bidirectional_flag> <organic_flag> <external_bandwidth> <power> <stackup>\nExiting...")
    sys.exit(1)


number_of_chiplets = int(sys.argv[1])
interchiplet_IO_type = sys.argv[2]
bidirectional = sys.argv[3]
organic = sys.argv[4]
total_ext_bandwidth = float(sys.argv[5])
total_power = float(sys.argv[6])
chip_stackup = sys.argv[7]

total_chiplet_area = 800

area_of_chiplets = total_chiplet_area / number_of_chiplets
power_per_chiplet = total_power / number_of_chiplets

identifier = "chiplet_" + str(number_of_chiplets) + "_" + str(math.floor(area_of_chiplets))

test_process = "test_process_0"
wafer_process = "process_1"
# wafer_diameter = "300"
# edge_exclusion = "3"
# wafer_process_yield = " 0.94"
# reticle_x = "26"
# reticle_y = "33"

# externalInputs = "0"
# externalOutputs = "0"

nre_design_cost = "0.0"
quantity="10000000"

if bidirectional == "True":
    bidirectional_flag = True
elif bidirectional == "False":
    bidirectional_flag = False
else:
    print("ERROR: Invalid bidirectional flag\nExiting...")
    sys.exit(1)

if organic == "True":
    organic_flag = True
elif organic == "False":
    organic_flag = False
else:
    print("ERROR: Invalid organic flag\nExiting...")
    sys.exit(1)

print("\t\t>>> Organic flag is " + str(organic_flag) + " <<<")

if organic_flag:
    assembly_process = "organic_simultaneous_bonding"
    interposer_stackup = "1:combined_interposer_organic"
else:
    assembly_process = "silicon_individual_bonding"
    interposer_stackup = "1:combined_interposer_silicon"

# Generate def xml file
def_file = open(identifier + "_def" + ".xml", "w")

def_file.write("<chip name=\"interposer\"\n")
def_file.write("\tcoreArea=\"0.0\"\n")
def_file.write("\tburied=\"False\"\n")
def_file.write("\tassembly_process=\"" + assembly_process + "\"\n")
def_file.write("\ttest_process=\"" + test_process + "\"\n")
def_file.write("\tstackup=\"" + interposer_stackup + "\"\n")
def_file.write("\twafer_process=\"" + wafer_process + "\"\n")
# def_file.write("\twafer_diameter=\"" + wafer_diameter + "\"\n")
# def_file.write("\tedge_exclusion=\"" + edge_exclusion + "\"\n")
# def_file.write("\twafer_process_yield=\"" + wafer_process_yield + "\"\n")
# def_file.write("\treticle_x=\"" + reticle_x + "\"\n")
# def_file.write("\treticle_y=\"" + reticle_y + "\"\n")
# def_file.write("\texternalInputs=\"" + externalInputs + "\"\n")
# def_file.write("\texternalOutputs=\"" + externalOutputs + "\"\n")
def_file.write("\tnre_design_cost=\"" + nre_design_cost + "\"\n")
def_file.write("\tpower=\"0.0\"\n")
def_file.write("\tquantity=\"" + quantity + "\"\n")
def_file.write("\tcore_voltage=\"1.0\">\n")

for i in range(number_of_chiplets):
    def_file.write("\t<chip name=\"chiplet_" + str(i) + "\"\n")
    def_file.write("\t\tcoreArea=\"" + str(area_of_chiplets) + "\"\n")
    def_file.write("\t\tburied=\"False\"\n")
    def_file.write("\t\tassembly_process=\"" + assembly_process + "\"\n")
    def_file.write("\t\ttest_process=\"" + test_process + "\"\n")
    def_file.write("\t\tstackup=\"" + chip_stackup + "\"\n")
    def_file.write("\t\twafer_process=\"" + wafer_process + "\"\n")
    # def_file.write("\t\twafer_diameter=\"" + wafer_diameter + "\"\n")
    # def_file.write("\t\tedge_exclusion=\"" + edge_exclusion + "\"\n")
    # def_file.write("\t\twafer_process_yield=\"" + wafer_process_yield + "\"\n")
    # def_file.write("\t\treticle_x=\"" + reticle_x + "\"\n")
    # def_file.write("\t\treticle_y=\"" + reticle_y + "\"\n")
    # def_file.write("\t\texternalInputs=\"" + externalInputs + "\"\n")
    # def_file.write("\t\texternalOutputs=\"" + externalOutputs + "\"\n")
    def_file.write("\t\tnre_design_cost=\"" + nre_design_cost + "\"\n")
    def_file.write("\t\tpower=\"" + str(power_per_chiplet) + "\"\n")
    def_file.write("\t\tquantity=\"" + quantity + "\"\n")
    def_file.write("\t\tcore_voltage=\"1.0\">\n")
    def_file.write("\t</chip>\n")

def_file.write("</chip>\n")
def_file.close()

# Generate netlist xml file
# Open netlist file
netlist_file = open(identifier + "_netlist" + ".xml", "w")
netlist_file.write("<netlist>\n")

edge_bandwidth = total_ext_bandwidth / (math.sqrt(number_of_chiplets) * 4)

for i in range(number_of_chiplets):
    if i == 0 or i == number_of_chiplets - 1 or i == math.sqrt(number_of_chiplets) - 1 or i == number_of_chiplets - math.sqrt(number_of_chiplets):
        # corner
        external_bandwidth = edge_bandwidth * 2
    elif i % math.sqrt(number_of_chiplets) == 0 or i % math.sqrt(number_of_chiplets) == math.sqrt(number_of_chiplets) - 1:
        # edge
        external_bandwidth = edge_bandwidth
    netlist_file.write("\t<net type=\"GPIO_external_small\"\n")
    netlist_file.write("\t\tblock0=\"chiplet_" + str(i) + "\"\n")
    netlist_file.write("\t\tblock1=\"external\"\n")
    netlist_file.write("\t\tbandwidth=\"" + str(external_bandwidth) + "\">\n")
    netlist_file.write("\t</net>\n")
    # Now the other direction
    netlist_file.write("\t<net type=\"GPIO_external_small\"\n")
    netlist_file.write("\t\tblock0=\"external\"\n")
    netlist_file.write("\t\tblock1=\"chiplet_" + str(i) + "\"\n")
    netlist_file.write("\t\tbandwidth=\"" + str(external_bandwidth) + "\">\n")
    netlist_file.write("\t</net>\n")

# Now define internal connections assuming a grid of squares with the same edge bandwidth between each adjacent chiplets.
for i in range(number_of_chiplets):
    if i % math.sqrt(number_of_chiplets) != math.sqrt(number_of_chiplets) - 1: # Connect to chiplet on the right
        netlist_file.write("\t<net type=\"" + interchiplet_IO_type + "\"\n")
        netlist_file.write("\t\tblock0=\"chiplet_" + str(i) + "\"\n")
        netlist_file.write("\t\tblock1=\"chiplet_" + str(i + 1) + "\"\n")
        netlist_file.write("\t\tbandwidth=\"" + str(edge_bandwidth) + "\">\n")
        netlist_file.write("\t</net>\n")
    if bidirectional_flag != True:
        if i % math.sqrt(number_of_chiplets) != 0: # Connect to chiplet on the left
            netlist_file.write("\t<net type=\"" + interchiplet_IO_type + "\"\n")
            netlist_file.write("\t\tblock0=\"chiplet_" + str(i) + "\"\n")
            netlist_file.write("\t\tblock1=\"chiplet_" + str(i - 1) + "\"\n")
            netlist_file.write("\t\tbandwidth=\"" + str(edge_bandwidth) + "\">\n")
            netlist_file.write("\t</net>\n")
    if i >= math.sqrt(number_of_chiplets): # Connect to chiplet above
        netlist_file.write("\t<net type=\"" + interchiplet_IO_type + "\"\n")
        netlist_file.write("\t\tblock0=\"chiplet_" + str(i) + "\"\n")
        netlist_file.write("\t\tblock1=\"chiplet_" + str(int(i - math.sqrt(number_of_chiplets))) + "\"\n")
        netlist_file.write("\t\tbandwidth=\"" + str(edge_bandwidth) + "\">\n")
        netlist_file.write("\t</net>\n")
    if bidirectional_flag != True:   
        if i < number_of_chiplets - math.sqrt(number_of_chiplets): # Connect to chiplet below
            netlist_file.write("\t<net type=\"" + interchiplet_IO_type + "\"\n")
            netlist_file.write("\t\tblock0=\"chiplet_" + str(i) + "\"\n")
            netlist_file.write("\t\tblock1=\"chiplet_" + str(int(i + math.sqrt(number_of_chiplets))) + "\"\n")
            netlist_file.write("\t\tbandwidth=\"" + str(edge_bandwidth) + "\">\n")
            netlist_file.write("\t</net>\n")


netlist_file.write("</netlist>\n")
netlist_file.close()
print("Generated " + identifier + "_def.xml and " + identifier + "_netlist.xml")