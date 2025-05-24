# ChipletPart

**ChipletPart** is an efficient framework designed for partitioning driven by chiplets technology. It aids in organizing and optimizing the design and layout of chiplets within a semiconductor module.
An example of an SoC netlist is the [Mempool testcase](https://github.com/bodhi91/ChipletPart/blob/main/MempoolGroupFromMempoolPaper.png). 

### Overall Flow ###
The overall flow is shown below: ![shown here](https://github.com/ABKGroup/ChipletPart/blob/main/chipletpart_overall_v5.png).

### Review Directions ###
1. The source codes to ChipletPart are in the directory ```graph_part```
2. The testcases are uploaded in the directory ```test_data```
3. The netlist generator file is implemented in the script ```generateSystemDefinition.py``` available in the directory ```test_case_gen```
To generate a testcase wih Z wafer scale, A tiles, B cores, C shared memories, D area scale and E power scale, we run the script:
```python3 systemSystemDefinition.py Z A B C D E```
4. ChipletPart has depencies on ```METIS```, ```PugiXML```, ```Eigen```, ```Boost```, ```OpenMP``` and ```GKLib```.
5. The run scripts are uploaded in the parent directory. 

### List of testcases ###

| Benchmark    | # Tiles | # IP Blocks | Area (mm²)          | Area Scaling | Power Scaling |
|--------------|---------|-------------|---------------------|--------------|---------------|
| WS₁          | 1       | 48          | 1582 (45nm)         | 1600         | 1600          |
| WS₂          | 2       | 96          | 3165 (45nm)         | 1600         | 1600          |
| WS₃          | 4       | 192         | 6330 (45nm)         | 1600         | 1600          |
| WS₄          | 8       | 384         | 12,660 (45nm)       | 1600         | 1600          |
| MP           | 16      | 40          | 494 (45nm)          | 100          | 100           |
| ADI₁         | N/A     | 14          | 567 (16nm)          | 16           | 16            |
| ADI₂         | N/A     | 17          | 567 (16nm)          | 16           | 16            |
| EPYC         | N/A     | 32          | 148 (7nm)           | 1            | 1             |
| GA100        | N/A     | 180         | 548 (7nm)           | 1            | 1             |

> **Note:** MP and WS benchmarks use tile terminology; the other benchmarks do not.

Some examples to run the test_case generator is below: 
1. To generate the WS_1 netlist run ```python3 generateSystemDefinition ws-48 1 14 4 1600 1600```
2. To generate the WS_2 netlist run ```python3 generateSystemDefinition.py ws-48 2 14 4 1600 1600```
3. To generate the WS_3 netlist run ```python3 generateSystemDefinition.py ws-48 4 14 4 1600 1600```
4. To generate the WS_4 netlist run ```python3generateSystemDefinition.py ws-48 8 14 4 1600 1600```

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
4. Run the run-script
   ```./run_chiplet_test.sh ga100 --seed 1```
5. Our cost model implementation is available in the ```cost_model``` directory.

ChipletPart operates in three modes: (1) homogeneous, (2) heterogeneous and (3) evaluation.
## Homogeneous mode ##
In this mode ChipletPart works with a single technology node. To run in homogeneous mode do the following:
```./run_chiplet_test.sh ga100 --seed 1```
To change the technology node, modify the default tech variable in run_chiplet_test.sh, e.g., ```DEFAULT_TECH="7nm"```

### Sample output ###
```
Running standard partitioning for test case: adi_sku3_1x
Tech node: 7nm
[INFO] Random seed set to 1

------------------------------------------------------------
            ChipletPart Partitioner / Evaluator             
                        Version: 1.0                        
------------------------------------------------------------
Developed by: UC San Diego and UC Los Angeles               
------------------------------------------------------------

[INFO] Random seed set to 1
[INFO] Partitioning using XML input files

Reading Chiplet Files and Generating Hypergraph
Parameter                     Value                                             
--------------------------------------------------------------------------------
IO file                       /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/io_definitions.xml
Layer file                    /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/layer_definitions.xml
Wafer process file            /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/wafer_process_definitions.xml
Assembly process file         /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/assembly_process_definitions.xml
Test file                     /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/test_definitions.xml
Netlist file                  /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/block_level_netlist.xml
Blocks file                   /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/block_definitions.txt
Reach                         0.500000                                          
Separation                    0.250000                                          
Technology                    7nm           
[==================================================] 100%
[SUCCESS] Parallel floorplanning and refinement completed in 17 seconds

Best Partition Results
Metric                        Value                         
------------------------------------------------------------
Partition index               6                             
Number of parts               4                             
Cost                          50.790554                     
Feasibility                   Yes                           
Aspect Ratios                 [4.677498, 0.338730, 0.200000, 0.200000]

Top 3 Partition Results by Cost:
Rank      Partition Index     Parts     Cost                Feasible  
----------------------------------------------------------------------
1         6                   4         50.790554           Yes       
2         7                   4         51.204288           Yes       
3         3                   2         52.260674           Yes       

[SUCCESS] Best partition saved to /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/block_level_netlist.xml.cpart.4

Timing Summary
Stage                                   Time (seconds)      
------------------------------------------------------------
Parallel floorplanning and refinement   17                  
Total partitioning process              17.163105           

[SUCCESS] Partitioning completed successfully
Test completed successfully!
```
## Heterogeneous mode ##
ChipletPart can handle multiple technology nodes -- in this case it will run the genetic algorithm. 
```./run_chiplet_test.sh ga100 --genetic --tech-nodes 7nm,10nm,45nm --seed 1```

### Sample output ###
```./run_chiplet_test.sh ga100 --genetic --tech-nodes 7nm,10nm,45nm --seed 1```

```
Running genetic tech partitioning for test case: adi_sku3_1x
Tech nodes: 7nm,10nm,45nm
Generations: 50, Population size: 50
/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/build/bin/chipletPart /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/io_definitions.xml /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/layer_definitions.xml /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/wafer_process_definitions.xml /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/assembly_process_definitions.xml /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/test_definitions.xml /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/block_level_netlist.xml /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/block_definitions.txt 0.50 0.25 --genetic-tech-part --tech-nodes 7nm,10nm,45nm --generations 50 --population 50 --seed 1
[INFO] Random seed set to 1

------------------------------------------------------------
            ChipletPart Partitioner / Evaluator             
                        Version: 1.0                        
------------------------------------------------------------
Developed by: UC San Diego and UC Los Angeles               
------------------------------------------------------------

[INFO] Random seed set to 1
[INFO] Converting netlist XML to hypergraph...
[INFO] Created hypergraph with 11 vertices and 20 hyperedges
[INFO] Number of IP blocks in chiplet graph: 11
[INFO] Number of nets in chiplet graph: 20
[INFO] Wrote vertex mapping to output.map

Running Enhanced Genetic Algorithm for Co-optimization
Parameter                     Value                                             
--------------------------------------------------------------------------------
IO file                       /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/io_definitions.xml
Layer file                    /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/layer_definitions.xml
Wafer process file            /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/wafer_process_definitions.xml
Assembly process file         /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/assembly_process_definitions.xml
Test file                     /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/test_definitions.xml
Netlist file                  /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/block_level_netlist.xml
Blocks file                   /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/adi_sku3_1x/block_definitions.txt
Reach                         0.500000                                          
Separation                    0.250000                                          
Population Size               50                                                
Number of Generations         50                                                
Mutation Rate                 0.200000                                          
Crossover Rate                0.650000                                          
Min Partitions                1                                                 
Max Partitions                8                                                 
Output Prefix                 genetic_tech_part                                 
Random Seed                   1                                                 

[INFO] Creating GeneticTechPartitioner with hypergraph containing 11 vertices and 20 hyperedges

Genetic Tech Partitioner Initialized
Parameter                     Value               
--------------------------------------------------
Hypergraph Vertices           11                  
Hypergraph Hyperedges         20                  
Available Technologies        3                   
Random Seed                   1                   
Generations                   50                  
Population Size               50                  
Mutation Rate                 0.200000            
Crossover Rate                0.650000            
Min Partitions                1                   
Max Partitions                8                   

[INFO] Running genetic algorithm...
Genetic Algorithm Results
Metric                        Value               
--------------------------------------------------
Best Cost                     43.749119           
Number of Partitions          6                   
Valid Solution                Yes                 
Execution Time (ms)           21384               

Technology Assignment
Partition                     Technology          
--------------------------------------------------
0                             7nm                 
1                             7nm                 
2                             7nm                 
3                             10nm                
4                             10nm                
5                             7nm                 


Genetic Algorithm Results
Metric                        Value                                             
--------------------------------------------------------------------------------
Best Cost                     43.749119                                         
Number of Partitions          6                                                 
Valid Solution                Yes                                               
Execution Time (seconds)      21   
```

## Evaluation mode ## 
ChipletPart can also evaluate a given partition solution. It will first floorplan it and then evaluate it with its cost model. 
```./run_evaluator.sh ga100``` -- you can mention the partition file with the script variable ```HYPERGRAPH_PART```.


### Sample output ###
```./run_evaluator.sh ga100 --seed 1```
   ```
   Running evaluator with the following parameters:
Partition file:        /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/manual.part
IO file:               /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/io_definitions.xml
Layer file:            /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/layer_definitions.xml
Wafer process file:    /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/wafer_process_definitions.xml
Assembly process file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/assembly_process_definitions.xml
Test file:             /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/test_definitions.xml
Netlist file:          /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/block_level_netlist.xml
Blocks file:           /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/block_definitions.txt
Reach:                 0.50
Separation:            0.25
Technology:            7nm

Executing evaluation...

------------------------------------------------------------
            ChipletPart Partitioner / Evaluator             
                        Version: 1.0                        
------------------------------------------------------------
Developed by: UC San Diego and UC Los Angeles               
------------------------------------------------------------

[INFO] Evaluating partition
[INFO] Reading chiplet files and generating hypergraph representation
[INFO] Partition file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/manual.part
[INFO] IO file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/io_definitions.xml
[INFO] Layer file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/layer_definitions.xml
[INFO] Wafer process file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/wafer_process_definitions.xml
[INFO] Assembly process file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/assembly_process_definitions.xml
[INFO] Test file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/test_definitions.xml
[INFO] Netlist file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/block_level_netlist.xml
[INFO] Blocks file: /home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/branch_JETCAS/ChipletPart/test_data/ga100/block_definitions.txt
[INFO] Reach: 0.5
[INFO] Separation: 0.25
[INFO] Tech: 7nm
[INFO] Converting netlist XML to hypergraph...
[INFO] Created hypergraph with 180 vertices and 400 hyperedges
[INFO] Number of IP blocks in chiplet graph: 180
[INFO] Number of nets in chiplet graph: 400
[INFO] Number of partitions: 2
[INFO] Creating ChipletRefiner with cost model files to test initialization
[SUCCESS] Cost model was successfully initialized in the ChipletRefiner constructor
Cost: 34.6704, Power: 0
[INFO] Cost of partition is 34.670376
[INFO] Number of partitions is 2
[INFO] Floorplan feasibility is 1
Evaluation completed successfully
```


## Usage Directions ##
  ```
Usage: ./run_chiplet_test.sh <test_case_name> [options]

Options:
  --reach <value>       Specify reach value (default: 0.50)
  --separation <value>  Specify separation value (default: 0.25)
  --tech <node>         Specify tech node for standard partitioning (default: 7nm)
  --seed <value>        Specify random seed (default: 42)
  --genetic             Use genetic tech partitioning algorithm
  --tech-nodes <nodes>  Specify comma-separated list of tech nodes for genetic partitioning
                        Example: --tech-nodes 7nm,10nm,14nm,28nm
  --generations <num>   Specify number of generations for genetic algorithm (default: 50)
  --population <num>    Specify population size for genetic algorithm (default: 50)
  --help                Display this help message

Examples:
  ./run_chiplet_test.sh design1 --tech 5nm
  ./run_chiplet_test.sh design2 --genetic --tech-nodes 7nm,10nm,14nm --seed 123
  ```

   
