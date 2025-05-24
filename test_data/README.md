# ChipletPart Test Data

This directory should contain XML files needed for chiplet partitioning. The files are used by the `run_chiplet_test.sh` script and the `chipletPart` executable.

## Required Files

The following files should be placed in this directory:

1. **io_definitions.xml** - Definition of IO types and their reach values
   - Example format:
   ```xml
   <ios>
     <io type="UCIe_standard" reach="1.5" />
     <io type="custom_io" reach="0.8" />
   </ios>
   ```

2. **layer_definitions.xml** - Layer definitions for chiplets
   - Example format: See cost_model/test_cases/48_1_14_4_1600_1600/layer_definitions.xml

3. **wafer_process_definitions.xml** - Wafer process definitions
   - Example format: See cost_model/test_cases/48_1_14_4_1600_1600/wafer_process_definitions.xml

4. **assembly_process_definitions.xml** - Assembly process definitions
   - Example format: See cost_model/test_cases/48_1_14_4_1600_1600/assembly_process_definitions.xml

5. **test_definitions.xml** - Test process definitions
   - Example format: See cost_model/test_cases/48_1_14_4_1600_1600/test_definitions.xml

6. **block_level_netlist.xml** - XML netlist file defining connections between blocks
   - Example format:
   ```xml
   <netlist>
     <net type="UCIe_standard" block0="0x" block1="1x" bandwidth="1" />
     <net type="UCIe_standard" block0="1x" block1="2x" bandwidth="1" />
     <net type="UCIe_standard" block0="2x" block1="3x" bandwidth="1" />
     <net type="UCIe_standard" block0="3x" block1="0x" bandwidth="1" />
   </netlist>
   ```

7. **block_definitions.txt** - Block definitions file with area, power, and technology information
   - Format (space-separated values):
   ```
   Block_Name Area Power Technology
   ```
   - Example:
   ```
   Master_Crossbar_0 0.041655 0.03 7nm
   Slave_Crossbar_1 0.038721 0.025 7nm
   ```

## Usage Notes

- You can reference example files in `cost_model/test_cases/48_1_14_4_1600_1600/` for format details
- The chipletPart executable will generate a hypergraph internally from these files
- Results will be saved in the `results/` directory at the project root

## Quick Start

Copy files from the cost_model/test_cases directory:

```bash
cp cost_model/test_cases/48_1_14_4_1600_1600/io_definitions.xml test_data/io_definitions.xml
cp cost_model/test_cases/48_1_14_4_1600_1600/layer_definitions.xml test_data/layer_definitions.xml
cp cost_model/test_cases/48_1_14_4_1600_1600/wafer_process_definitions.xml test_data/wafer_process_definitions.xml
cp cost_model/test_cases/48_1_14_4_1600_1600/assembly_process_definitions.xml test_data/assembly_process_definitions.xml
cp cost_model/test_cases/48_1_14_4_1600_1600/test_definitions.xml test_data/test_definitions.xml
cp cost_model/test_cases/48_1_14_4_1600_1600/block_level_netlist_ws-48_1_14_4_1600_1600.xml test_data/block_level_netlist.xml
cp cost_model/test_cases/48_1_14_4_1600_1600/block_definitions_ws-48_1_14_4_1600_1600.txt test_data/block_definitions.txt
```

Or simply run the `setup_test_data.sh` script which will copy these files for you. 