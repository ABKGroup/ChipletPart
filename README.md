# ChipletPart

**ChipletPart** is an efficient framework designed for partitioning driven by chiplets technology. It aids in organizing and optimizing the design and layout of chiplets within a semiconductor module.
An example of an SoC netlist is the [Mempool testcase](https://github.com/bodhi91/ChipletPart/blob/main/MempoolGroupFromMempoolPaper.png) refered to as [34] from the our submission. 

### Overall Flow ###
The overall flow is shown below: ![shown here](https://github.com/bodhi91/ChipletPart/blob/main/ChipletPart_flow_new.png).

### Review Directions ###
1. The source codes to ChipletPart are in the directory ```graph_part```
2. The testcases are uploaded in the directory ```testcases```
3. The netlist generator file is implemented in the script ```generateSystemDefinitionMemCPUScale.py```
To generate a testcase wih Z wafer scale, A tiles, B cores, C shared memories, D area scale and E power scale, we run the script:
```python3 systemMempoolSystemDefinition.py Z A B C D E```
5. To generate the hypergraph file from the SoC netlist, use the script ```xml_to_hypergraph.py```
6. The source codes of ChipletPart are uploaded in the directory ```graph_part```. ChipletPart has depencies on ```METIS```, ```Python``` and ```GKLib```.
7. The run scripts are uploaded in the directory ```test```. 

### List of testcases ###

| Testcase | # Tiles | # IP blocks | Area (mm²) | Area scaling | Power scaling |
|----------|---------|-------------|------------|--------------|---------------|
| WS_1     | 4       | 192         | 396        | 400          | 1             |
| WS_2     | 4       | 192         | 791        | 800          | 800           |
| WS_3     | 1       | 48          | 396        | 1600         | 1600          |
| WS_4     | 1       | 48          | 791        | 3200         | 3200          |
| MP       | 16      | 36          | 494        | 100          | 100           |

1. To generate the WS_1 netlist run ```python3 generateSystemDefinitionMemCPUScale.py ws-48 4 14 4 400 1```
2. To generate the WS_2 netlist run ```python3 generateSystemDefinitionMemCPUScale.py ws-48 4 14 4 800 800```
3. To generate the WS_3 netlist run ```python3 generateSystemDefinitionMemCPUScale.py ws-48 1 14 4 1600 1600```
4. To generate the WS_4 netlist run ```python3generateSystemDefinitionMemCPUScale.py ws-48 1 14 4 3200 3200```
5. To generate the MP netlist run ```python3 systemMempoolSystemDefinition.py ws-48 100 100```

## Building ChipletPart

Follow these steps to build the ChipletPart from the source:

### Prerequisites

Ensure you have CMake 3.10 or higher and a suitable (9.3.0 or higher) C++ compiler installed on your system.

### Build Instructions

1. **Create and Enter Build Directory:**
   ```bash
   mkdir build && cd build
   ```
2. **Generate Makefiles:**
   ```cmake ..```
3. Compile the Project:
   ```make -j 40  # Adjust the number 40 according to the number of cores you wish to use for a faster build process```  
4. Navigate to the Test Directory:
   ```cd ../test/```
6. Run the Application:
   ```
   ./build/chipletPart \
   ../testcases/48_4_14_4_400_1/block_level_netlist_ws-48_4_14_4_400_1.hgr \
   ../testcases/48_4_14_4_400_1/io_definitions.xml \
   ../testcases/48_4_14_4_400_1/layer_definitions.xml \
   ../testcases/48_4_14_4_400_1/wafer_process_definitions.xml \
   ../testcases/48_4_14_4_400_1/assembly_process_definitions.xml \
   ../testcases/48_4_14_4_400_1/test_definitions.xml \
   ../testcases/48_4_14_4_400_1/block_level_netlist_ws-48_4_14_4_400_1.xml \
   ../testcases/48_4_14_4_400_1/block_definitions_ws-48_4_14_4_400_1.txt \
   2 0.1 "45nm,10nm,7nm"```

7. Sample output:
   ```
   [INFO] Reading the chiplet graph ../testcases/48_4_14_4_400_1/block_level_netlist_ws-48_4_14_4_400_1.hgr
   [INFO] Number of IP blocks in chiplet graph: 192
   [INFO] Number of nets in chiplet graph: 192
   [INFO] Evaluating the partitioning of the chiplet graph ../testcases/48_4_14_4_400_1/block_level_netlist_ws-48_4_14_4_400_1.hgr
   [INFO] Reading the chiplet graph ../testcases/48_4_14_4_400_1/block_level_netlist_ws-48_4_14_4_400_1.hgr
   [INFO] Reading the partition file ../testcases/48_4_14_4_400_1/block_level_netlist_ws-48_4_14_4_400_1_hmetis.hgr.part.16
   [INFO] Reading the chiplet IO file ../testcases/48_4_14_4_400_1/io_definitions.xml
   [INFO] Reading the chiplet layer file ../testcases/48_4_14_4_400_1/layer_definitions.xml
   [INFO] Reading the chiplet wafer process file ../testcases/48_4_14_4_400_1/wafer_process_definitions.xml
   [INFO] Reading the chiplet assembly process file ../testcases/48_4_14_4_400_1/assembly_process_definitions.xml
   [INFO] Reading the chiplet test file ../testcases/48_4_14_4_400_1/test_definitions.xml
   [INFO] Reading the chiplet netlist file ../testcases/48_4_14_4_400_1/block_level_netlist_ws-48_4_14_4_400_1.xml
   [INFO] Reading the chiplet blocks file ../testcases/48_4_14_4_400_1/block_definitions_ws-48_4_14_4_400_1.txt
   [INFO] Reach: 2
   [INFO] Separation: 0.1
   [INFO] Tech: 45nm
   [INFO] Number of partitions: 16
   [INFO] Aspect ratios: 1.14348 0.0412312 0.470963 0.0211852 7.55237 0.223703 0.0247888 2.88606 20.8536 0.838295 0.0980606 0.521429 0.391418 0.478707 0.934899 47.8078 
   Floorplan is feasible: 1
   [COST-FPL-INFO] [Partition has cost 703.101] [FP feasible 1]
   ```


  ### Usage Directions
  ```
Usage for partitioning: ../build/chipletPart <hypergraph_file> <chiplet_io_file> <chiplet_layer_file> <chiplet_wafer_process_file> <chiplet_assembly_process_file> <chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> <reach> <separation> <tech>
Usage for evaluation: ../build/chipletPart <hypergraph_file> <hypergraph_part> <chiplet_io_file> <chiplet_layer_file> <chiplet_wafer_process_file> <chiplet_assembly_process_file> <chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> <reach> <separation> <tech>
Usage for technology assignment: ../build/chipletPart <hypergraph_file> <hypergraph_part> <chiplet_io_file> <chiplet_layer_file> <chiplet_wafer_process_file> <chiplet_assembly_process_file> <chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> <reach> <separation> <tech array>

   
