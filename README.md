# ChipletPart

**ChipletPart** is an efficient framework designed for partitioning driven by chiplets technology. It aids in organizing and optimizing the design and layout of chiplets within a semiconductor module.
An example of an SoC netlist is the [Mempool testcase](https://github.com/bodhi91/ChipletPart/blob/main/MempoolGroupFromMempoolPaper.png). 

### Overall Flow ###
The overall flow is shown below: ![shown here](https://github.com/ABKGroup/ChipletPart/blob/main/chipletpart_overall_v5.png).

### Repository layout ###
1. `src/` – the partitioner (`ChipletPart`, FM/KL refiners, sequence-pair floorplanner, GAs, CLI in `Main.cpp`).
2. `cost_model/` – the C++ chiplet cost model (`evaluator_cpp`, `design::*`) and its unit tests.
3. `test_data/<case>/` – one directory per benchmark, each containing `io_definitions.xml`, `layer_definitions.xml`,
   `wafer_process_definitions.xml`, `assembly_process_definitions.xml`, `test_definitions.xml`,
   `block_level_netlist.xml` and `block_definitions.txt` (columns: `name area_mm2 power_W tech is_memory`).
4. `test_case_gen/generateSystemDefinition.py` – the wafer-scale (WS) netlist generator.
5. `run_*.sh` – run scripts (see [Usage](#usage-directions)). Results go to `results/`; `test_data/` is never written to.

Dependencies: CMake ≥ 3.14, a C++17 compiler, OpenMP, Boost (headers), Eigen 3, METIS 5 (+ GKlib) and pugixml (bundled in `src/`).

### List of testcases ###

| Benchmark | Directory | # Tiles | # IP Blocks | Area (mm²) | Area Scaling | Power Scaling |
|-----------|-----------|---------|-------------|------------|--------------|---------------|
| WS₁ | `48_1_14_4_1600_1600` | 1 | 48  | 1582 (45nm)   | 1600 | 1600 |
| WS₂ | `48_2_14_4_1600_1600` | 2 | 96  | 3165 (45nm)   | 1600 | 1600 |
| WS₃ | `48_4_14_4_1600_1600` | 4 | 192 | 6330 (45nm)   | 1600 | 1600 |
| WS₄ | `48_8_14_4_1600_1600` | 8 | 384 | 12,660 (45nm) | 1600 | 1600 |
| MP  | `mempool_group`       | 16 | 40 | 494 (45nm)    | 100  | 100  |
| EPYC | `epyc7282`           | N/A | 32 | 148 (7nm)     | 1    | 1    |
| GA100 | `ga100`             | N/A | 180 | 548 (7nm)    | 1    | 1    |

> **Note:** MP and WS benchmarks use tile terminology; the other benchmarks do not.

To generate a WS testcase with `Z` wafer scale, `A` tiles, `B` cores, `C` shared memories, `D` area scale and `E` power scale:
```bash
cd test_case_gen
python3 generateSystemDefinition.py ws-48 1 14 4 1600 1600   # WS_1
python3 generateSystemDefinition.py ws-48 2 14 4 1600 1600   # WS_2
python3 generateSystemDefinition.py ws-48 4 14 4 1600 1600   # WS_3
python3 generateSystemDefinition.py ws-48 8 14 4 1600 1600   # WS_4
```

## Building ChipletPart

### Prerequisites

CMake ≥ 3.14 and a C++17 compiler (GCC ≥ 9 or Apple clang).

```bash
# macOS
brew install cmake libomp boost eigen metis
# Debian / Ubuntu
sudo apt install cmake g++ libomp-dev libboost-dev libeigen3-dev libmetis-dev
```

METIS is found automatically from Homebrew / the system (`-DMETIS_PREFIX=<prefix>` to override).
To build METIS from source instead, `git submodule update --init` and CMake will use `METIS/` and `GKlib/`.
`-DDISABLE_METIS=ON` builds without METIS (random initial partitions).

### Build Instructions

```bash
mkdir build && cd build
cmake ..                      # Release by default
make -j8
cd ..
./run_chiplet_test.sh ga100 --seed 1
```

Useful CMake options: `-DENABLE_NATIVE_ARCH=ON` (`-march=native`, binary is then not portable),
`-DBUILD_COST_MODEL_TESTS=ON` (then `ctest` in `build/` runs the cost-model unit tests),
`-DBUILD_COST_MODEL_EXAMPLES=ON`, `-DENABLE_OPENMP=OFF`.

ChipletPart operates in four modes: (1) homogeneous, (2) heterogeneous (GA), (3) technology enumeration and (4) evaluation.

## Homogeneous mode ##
A single technology node for every chiplet:
```bash
./run_chiplet_test.sh ga100 --seed 1            # default tech 7nm
./run_chiplet_test.sh ga100 --tech 14nm
```
Results are written to `results/<case>/standard/` (override with `--output-dir`).

### Sample output (epyc7282, 7nm, seed 42) ###
```
+------------+----------------------+------------+----------------------+------------+
| Rank       | Partition Index      | Parts      | Cost                 | Feasible   |
+------------+----------------------+------------+----------------------+------------+
| 1          | 16                   | 8          | 71.835381            | Yes        |
| 2          | 17                   | 8          | 72.034363            | Yes        |
| 3          | 12                   | 6          | 73.213936            | Yes        |
[INFO] Floorplanner results: Yes
[SUCCESS] Best cost: 71.835381
[SUCCESS] Number of partitions: 8
[SUCCESS] Floorplan feasibility: 1
[SUCCESS] Best partition saved to results/epyc7282/standard/block_level_netlist.xml.cpart.8
```

### Floorplan feasibility ###
A partition is *feasible* when the sequence-pair floorplanner can place the chiplets (with the
`<separation>` halo and the IO shoreline each inter-chiplet bundle needs) so that every bundle is
within its IO `reach`. `Floorplanner results` / `Floorplan feasibility` report this for the final
partition and are authoritative; the `Feasible` column is the result of the shorter per-candidate run.

By default the refinement is **floorplan-aware**: every FM move and KL swap is followed by a short,
warm-started floorplan probe of the resulting partition, each pass is rolled back to the prefix with
the lowest reach violation (then the highest gain), and candidates are ranked feasible-first. The
cost of the winning partition is therefore the best cost *among feasible designs*; a cheaper but
unplaceable partition (e.g. EPYC at 70.83 with 8 chiplets) is not reported. The best arrangement the
probes found seeds the final floorplanner, which runs `--fp-steps x --fp-perturbs` perturbations per
walker on `--fp-workers` parallel walkers (default 1000 x 1000 on min(threads, 4) walkers).
`--no-floorplan-aware` restores pure cost-driven refinement (faster, may be infeasible);
`--fp-check-budget S,P` sets the probe budget (default `40,25`).

Results with the default settings (7nm, reach 0.5, separation 0.25, seed 42; "cost-only" is
`--no-floorplan-aware`):

| Case | Floorplan-aware: cost / chiplets / feasible | Cost-only: cost / chiplets / feasible |
|------|---------------------------------------------|---------------------------------------|
| EPYC (`epyc7282`) | 71.84 / 8 / yes | 70.83 / 8 / **no** |
| GA100 (`ga100`) | 70.31 / 8 / yes | 70.14 / 8 / **no** |
| WS₁ (`48_1_14_4_1600_1600`) | 49.89 / 7 / yes | 49.89 / 7 / yes |
| WS₃ (`48_4_14_4_1600_1600`) | 260.23 / 8 / yes | 260.23 / 8 / yes |
| MP (`mempool_group`) | 5.82 / 1 / yes | 5.82 / 1 / yes |

## Heterogeneous mode ##
Multiple technology nodes; partitioning and technology assignment are co-optimised by a genetic algorithm:
```bash
./run_chiplet_test.sh ga100 --genetic --tech-nodes 7nm,10nm,14nm --seed 1
./run_chiplet_test.sh ga100 --canonical-ga --tech-nodes 7nm,14nm --generations 30 --population 20
```
Both print a `Genetic Algorithm Results` table (`Best Cost`, `Number of Partitions`, per-partition technology)
and write `*.parts.N` / `*.techs.N` files to `results/<case>/<mode>/`.

## Technology enumeration ##
For small numbers of technologies and chiplets the search space is tiny (e.g. 5 unique canonical assignments for
2 technologies × ≤ 2 chiplets), so exhaustive enumeration is cheaper and deterministic compared with the GAs:
```bash
./run_chiplet_test.sh epyc7282 --tech-enum --tech-nodes 7nm,14nm --max-partitions 2
```
```
| Total Assignments Considered   | 6      |
| Unique Canonical Assignments   | 5      |
| Best Number of Partitions      | 2      |
| Best Technology Assignment     | [7nm, 14nm] |
| Best Cost                      | 85.264297   |
[SUCCESS] Best partition saved to results/epyc7282/tech_enum/block_level_netlist.xml.best_partition.txt
```

## Evaluation mode ##
Evaluate an existing partition (one chiplet id per line, in block order): it is floorplanned first and then costed.
```bash
./run_evaluator.sh ga100 --partition results/ga100/standard/block_level_netlist.xml.cpart.8 --tech 7nm
# equivalently
./run_chiplet_test.sh ga100 --evaluate-partition results/ga100/standard/block_level_netlist.xml.cpart.8
```
```
[INFO] Cost of partition is 71.915916
[INFO] Number of partitions is 8
[INFO] Floorplan feasibility is 1
```
The evaluation cost is computed with the aspect ratios and locations of the final floorplan, so it is
slightly different from the `Best cost` printed by the partitioner (which costs the partition before
floorplanning); compare partitions within one mode.

## Usage Directions ##
  ```
Usage: ./run_chiplet_test.sh <test_case_name> [options]

Modes (pick at most one; default is standard single-technology partitioning):
  --genetic                     Genetic tech partitioning (GeneticTechPartitioner)
  --canonical-ga                Canonical genetic algorithm for technology assignment
  --tech-enum                   Enumerate all canonical technology assignments
  --evaluate-partition <file>   Evaluate an existing partition file

Options:
  --reach <value>        Default IO reach for net types missing from the IO file (default: 0.50)
  --separation <value>   Minimum chiplet spacing used by the floorplanner (default: 0.25)
  --tech <node>          Tech node for standard partitioning / evaluation (default: 7nm)
  --tech-nodes <list>    Comma-separated tech nodes for --genetic/--canonical-ga/--tech-enum
                         (default: 7nm,10nm,14nm; known nodes: 45nm 16nm 14nm 10nm 7nm)
  --seed <value>         Random seed (default: 42)
  --generations <num>    GA generations (default: 50)
  --population <num>     GA population size (default: 50)
  --max-partitions <n>   Max chiplets (default: 4 for --tech-enum, 8 for the GAs)
  --detailed-output      Extra per-assignment output for --tech-enum
  --output-dir <dir>     Where to write results (default: results/<case>/<mode>)
  --no-floorplan-aware   Disable feasibility-gated refinement (standard mode)
  --fp-check-budget S,P  Per-move floorplan probe budget (default: 40,25)
  --fp-steps <n>         Final floorplan SA steps per walker (default: 1000)
  --fp-perturbs <n>      Final floorplan perturbations per step (default: 1000)
  --fp-workers <n>       Final floorplan parallel walkers (default: min(threads,4))
  --help                 Show this message

Examples:
  ./run_chiplet_test.sh 48_1_14_4_1600_1600 --tech 7nm
  ./run_chiplet_test.sh epyc7282 --genetic --tech-nodes 7nm,10nm,14nm --seed 123
  ./run_chiplet_test.sh epyc7282 --canonical-ga --tech-nodes 7nm,14nm --generations 30 --population 20
  ./run_chiplet_test.sh ga100 --tech-enum --tech-nodes 7nm,10nm,14nm --max-partitions 3
  ./run_chiplet_test.sh ga100 --evaluate-partition results/ga100/standard/block_level_netlist.xml.cpart.8
  ```

The binary itself (`build/bin/chipletPart --help`) takes the same options; per-net IO reach always
comes from the `reach` attribute in `io_definitions.xml`, and the `<reach>` positional is only the
fallback for net types that are not listed there. The cost model knows the technology nodes
45nm, 16nm, 14nm, 10nm and 7nm; any other node name is rejected.

   
