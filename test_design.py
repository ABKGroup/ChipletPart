import design as d
import numpy as np
import copy
import readDesignFromFile as readDesign

io_list = readDesign.ioDefinitionListFromFile("io_definitions.xml")

#for io in io_list:
#    print("========================================")
#    io.print_description()

layer_list = readDesign.layerDefinitionListFromFile("layer_definitions.xml")

#for layer in layer_list:
#    print("========================================")
#    layer.print_description()

assembly_process_list = readDesign.assemblyProcessDefinitionListFromFile("assembly_process_definitions.xml")

#for assembly_process in assembly_process_list:
#    print("========================================")
#    assembly_process.print_description()

#    print(assembly_process.get_name())
#    for i in range(assembly_process.get_machine_cost_list_len()):
#        print(assembly_process.get_machine_cost(i))
#    print(assembly_process.get_machine_cost_list())
#    for i in range(assembly_process.get_machine_lifetime_list_len()):
#        print(assembly_process.get_machine_lifetime(i))
#    print(assembly_process.get_machine_lifetime_list())
#    for i in range(assembly_process.get_machine_uptime_list_len()):
#        print(assembly_process.get_machine_uptime(i))
#    print(assembly_process.get_machine_uptime_list())
#    for i in range(assembly_process.get_technician_yearly_cost_list_len()):
#        print(assembly_process.get_technician_yearly_cost(i))
#    print(assembly_process.get_technician_yearly_cost_list())
#    print(assembly_process.get_materials_cost_per_mm2())
#    print(assembly_process.get_assembly_type())
#    print(assembly_process.get_picknplace_time())
#    print(assembly_process.get_picknplace_group())
#    print(assembly_process.get_bonding_time())
#    print(assembly_process.get_bonding_group())
#    print(assembly_process.get_static())


test_process_list = readDesign.testProcessDefinitionListFromFile("test_definitions.xml")

#for test_process in test_process_list:
#    print("===========================================")
#    print(test_process.get_name())

am, names = readDesign.globalAdjacencyMatrixFromFile("netlist.xml",io_list)


#for a in am:
#    print("===========================================")
#    print(a)
#    print(str(am[a]))


sip = d.Chip(filename="sip.xml",dict={},assemblyProcessList=assembly_process_list,testProcessList=test_process_list,layers=layer_list,ios=io_list,adjacency_matrix_definitions=am,block_names=names,static=False)


sip.print_description()

## Define and test the IO class.
#
#io = d.IO("UCIe_advanced",0.75,1.2,32,140,False) # self, name=None, type=None, area=None, shoreline=None, bandwidth=None, wire_count=None, static=True
#
#print(io.get_type())
#print(io.get_area())
#print(io.get_shoreline())
#print(io.get_bandwidth())
#print(io.get_wire_count())
#print(io.get_static())
#
#io.set_type("UCIe_standard")
#io.set_area(1.0)
#io.set_shoreline(1.5)
#io.set_bandwidth(16)
#io.set_wire_count(44)
#io.set_static()
#
#print(io.get_type())
#print(io.get_area())
#print(io.get_shoreline())
#print(io.get_bandwidth())
#print(io.get_wire_count())
#print(io.get_static())
#
#io_list = [io]
#
#
#block_names = ["block0","block1","block2","block3"]
#am = np.array([[0,1,1,0],[1,0,1,0],[1,1,0,1],[0,0,1,0]])
## Define and test the InterconnectAdjacencyMatrix class.
#a = d.InterconnectAdjacencyMatrix("UCIe_standard",io_list,block_names,copy.deepcopy(am)) # type=None, IO_list=None, block_names=None, adjacency_matrix=None, static=False
#
#print(a.get_type())
#print(a.get_block_names_len())
#for i in range(a.get_block_names_len()):
#    print(a.get_block_names_entry(i))
#print(a.get_block_names())
#print(a.get_adjacency_matrix_shape())
#for i in range(a.get_adjacency_matrix_shape()[0]):
#    for j in range(a.get_adjacency_matrix_shape()[1]):
#        print(a.get_adjacency_matrix_entry(i,j))
#print(a.get_adjacency_matrix())
#
#print(a.get_static())
#
#a.set_block_names_entry(0,"cpu")
#print(a.get_block_names())
#new_block_names = ["cpu","mem","gpu","nic"]
#a.set_block_names(new_block_names)
#print(a.get_block_names())
#a.set_adjacency_matrix_entry(1,0,2)
#a.set_adjacency_matrix_entry(0,1,2)
#print(a.get_adjacency_matrix())
#a.set_adjacency_matrix(copy.deepcopy(am))
#print(a.get_adjacency_matrix())
#
#a.combine_blocks(0,1)
#print(a.get_block_names())
#print(a.get_adjacency_matrix())
#print(a.get_adjacency_matrix_shape())
#
#
#
## Define and test the Layer class.
#active_7nm_layer = d.Layer("active_7nm",True,5,0.007,0.5,2,10000,0.99,False) # name=None, active=None, cost_per_mm2=None, defect_density=None, critical_area_ratio=None, clustering_factor=None, mask_cost=None, stitching_yield=None, static=True
#print(active_7nm_layer.get_name())
#print(active_7nm_layer.get_active())
#print(active_7nm_layer.get_cost_per_mm2())
#print(active_7nm_layer.get_defect_density())
#print(active_7nm_layer.get_critical_area_ratio())
#print(active_7nm_layer.get_clustering_factor())
#print(active_7nm_layer.get_mask_cost())
#print(active_7nm_layer.get_stitching_yield())
#print(active_7nm_layer.get_static())
#
#active_7nm_layer.set_name("passive_7nm_m1")
#active_7nm_layer.set_active(False)
#active_7nm_layer.set_cost_per_mm2(0.2)
#active_7nm_layer.set_defect_density(0.005)
#active_7nm_layer.set_critical_area_ratio(0.4)
#active_7nm_layer.set_clustering_factor(2)
#active_7nm_layer.set_mask_cost(5000)
#active_7nm_layer.set_stitching_yield(0.999)
#active_7nm_layer.set_static()
#
#print(active_7nm_layer.get_name())
#print(active_7nm_layer.get_active())
#print(active_7nm_layer.get_cost_per_mm2())
#print(active_7nm_layer.get_defect_density())
#print(active_7nm_layer.get_critical_area_ratio())
#print(active_7nm_layer.get_clustering_factor())
#print(active_7nm_layer.get_mask_cost())
#print(active_7nm_layer.get_stitching_yield())
#print(active_7nm_layer.get_static())
#
#print(active_7nm_layer.layerYield(100))
#
#
#machine_cost_list = [1000000,2000000,3000000]
#machine_lifetime_list = [5,10,15]
#machine_uptime_list = [0.9,0.95,0.99]
#technician_yearly_cost_list = [100000,200000,100000,50000,50000]
#
## Define and test the Assembly class
#assemblyProcess = d.Assembly("OrganicInterposer2.5D",machine_cost_list,machine_lifetime_list,machine_uptime_list,technician_yearly_cost_list,0.1,"D2D",10,1,20,1,False) # name=None, machine_cost_list=None, machine_lifetime_list=None, machine_uptime_list=None, technician_yearly_cost_list=None, materials_cost_per_mm2=None, assembly_type=None, picknplace_time=None, picknplace_group=None, bonding_time=None, bonding_group=None, static=True:
#
#print(assemblyProcess.get_name())
#print(assemblyProcess.get_machine_cost_list_len())
#for i in range(assemblyProcess.get_machine_cost_list_len()):
#    print(assemblyProcess.get_machine_cost(i))
#
#print(assemblyProcess.get_machine_cost_list())
#
#print(assemblyProcess.get_machine_lifetime_list_len())
#for i in range(assemblyProcess.get_machine_lifetime_list_len()):
#    print(assemblyProcess.get_machine_lifetime(i))
#
#print(assemblyProcess.get_machine_lifetime_list())
#
#print(assemblyProcess.get_machine_uptime_list_len())
#for i in range(assemblyProcess.get_machine_uptime_list_len()):
#    print(assemblyProcess.get_machine_uptime(i))
#
#print(assemblyProcess.get_machine_uptime_list())
#
#print(assemblyProcess.get_technician_yearly_cost_list_len())
#for i in range(assemblyProcess.get_technician_yearly_cost_list_len()):
#    print(assemblyProcess.get_technician_yearly_cost(i))
#
#print(assemblyProcess.get_technician_yearly_cost_list())
#
#print(assemblyProcess.get_materials_cost_per_mm2())
#print(assemblyProcess.get_assembly_type())
#print(assemblyProcess.get_picknplace_time())
#print(assemblyProcess.get_picknplace_group())
#print(assemblyProcess.get_bonding_time())
#print(assemblyProcess.get_bonding_group())
#print(assemblyProcess.get_static())
#
#
#
#assemblyProcess.set_name("SiliconInterposer2.5D")
#assemblyProcess.set_machine_cost_list([1000000,2000000,3000000,4000000])
#assemblyProcess.set_machine_lifetime_list([5,10,15,20])
#assemblyProcess.set_machine_uptime_list([0.9,0.95,0.99,0.999])
#assemblyProcess.set_technician_yearly_cost_list([100000,200000,100000,50000,50000,50000])
#assemblyProcess.set_materials_cost_per_mm2(0.2)
#assemblyProcess.set_assembly_type("D2W")
#assemblyProcess.set_picknplace_time(5)
#assemblyProcess.set_picknplace_group(1)
#assemblyProcess.set_bonding_time(10)
#assemblyProcess.set_bonding_group(1)
#assemblyProcess.set_static()
#
#print(assemblyProcess.get_name())
#print(assemblyProcess.get_machine_cost_list_len())
#print(assemblyProcess.get_machine_cost_list())
#print(assemblyProcess.get_machine_lifetime_list_len())
#print(assemblyProcess.get_machine_lifetime_list())
#print(assemblyProcess.get_machine_uptime_list_len())
#print(assemblyProcess.get_machine_uptime_list())
#print(assemblyProcess.get_technician_yearly_cost_list_len())
#print(assemblyProcess.get_technician_yearly_cost_list())
#print(assemblyProcess.get_materials_cost_per_mm2())
#print(assemblyProcess.get_assembly_type())
#print(assemblyProcess.get_picknplace_time())
#print(assemblyProcess.get_picknplace_group())
#print(assemblyProcess.get_bonding_time())
#print(assemblyProcess.get_bonding_group())
#print(assemblyProcess.get_static())
#
#
## Define and test the Test class.
#test_definition = d.Test("Test1",False)
#print(test_definition.get_name())
#print(test_definition.get_static())
#print(test_definition.set_name("Test2"))
#test_definition.set_static()
#print(test_definition.get_name())
#print(test_definition.get_static())
#
#
#layers = [active_7nm_layer]
#stackup_names = ["active_7nm_layer"]
#chip_definitions = []
#adjacency_matrix_definitions = []
#
## Define and test the Chip class.
## name=None, coreArea=None, quality=None, assemblyProcess=None, layers=None, stackup_names=None, chip_definitions=None,
##  ios=None, adjacency_matrix_definitions=None, externalInputs=None, externalOutputs=None, powerPins=None, groundPins=None
#chip0 = d.Chip("compute",100.0,0.999,assemblyProcess,layers,stackup_names,chip_definitions,
#               io_list,adjacency_matrix_definitions,10,10,10,10)
#
#print(chip0.get_name())
#print(chip0.get_coreArea())
#print(chip0.get_quality())
#print(chip0.get_assemblyProcess())
#print(chip0.get_stackup())
#print(chip0.get_externalInputs())
#print(chip0.get_externalOutputs())
#print(chip0.get_powerPins())
#print(chip0.get_groundPins())
#
#chip0.set_name("memory")
#chip0.set_coreArea(200.0)
#chip0.set_quality(0.9999)
#chip0.set_assemblyProcess(assemblyProcess)
#chip0.set_stackup(layers)
#chip0.set_chip_definitions(chip_definitions)
#chip0.set_externalInputs(20)
#chip0.set_externalOutputs(20)
#chip0.set_powerPins(20)
#chip0.set_groundPins(20)
#
#print(chip0.get_name())
#print(chip0.get_coreArea())
#print(chip0.get_quality())
#print(chip0.get_assemblyProcess())
#print(chip0.get_stackup())
#print(chip0.get_externalInputs())
#print(chip0.get_externalOutputs())
#print(chip0.get_powerPins())
#print(chip0.set_groundPins())
#
#