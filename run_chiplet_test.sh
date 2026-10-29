#!/bin/bash
#
# Run ChipletPart on one of the cases under test_data/.
#
# All result files are written to results/<case>/<mode>/ (override with
# --output-dir). The test_data/ directory is never written to.

set -u
set -o pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

show_help() {
    echo -e "${BLUE}Usage: $0 <test_case_name> [options]${NC}"
    echo
    echo "Modes (pick at most one; default is standard single-technology partitioning):"
    echo "  --genetic                     Genetic tech partitioning (GeneticTechPartitioner)"
    echo "  --canonical-ga                Canonical genetic algorithm for technology assignment"
    echo "  --tech-enum                   Enumerate all canonical technology assignments"
    echo "  --evaluate-partition <file>   Evaluate an existing partition file"
    echo
    echo "Options:"
    echo "  --reach <value>        Default IO reach for net types missing from the IO file (default: 0.50)"
    echo "  --separation <value>   Minimum chiplet spacing used by the floorplanner (default: 0.25)"
    echo "  --tech <node>          Tech node for standard partitioning / evaluation (default: 7nm)"
    echo "  --tech-nodes <list>    Comma-separated tech nodes for --genetic/--canonical-ga/--tech-enum"
    echo "                         (default: 7nm,10nm,14nm; known nodes: 45nm 16nm 14nm 10nm 7nm)"
    echo "  --seed <value>         Random seed (default: 42)"
    echo "  --generations <num>    GA generations (default: 50)"
    echo "  --population <num>     GA population size (default: 50)"
    echo "  --max-partitions <n>   Max chiplets (default: 4 for --tech-enum, 8 for the GAs)"
    echo "  --detailed-output      Extra per-assignment output for --tech-enum"
    echo "  --output-dir <dir>     Where to write results (default: results/<case>/<mode>)"
    echo "  --no-floorplan-aware   Disable feasibility-gated refinement (standard mode)"
    echo "  --fp-check-budget S,P  Per-move floorplan probe budget (default: 40,25)"
    echo "  --fp-steps <n>         Final floorplan SA steps per walker (default: 1000)"
    echo "  --fp-perturbs <n>      Final floorplan perturbations per step (default: 1000)"
    echo "  --fp-workers <n>       Final floorplan parallel walkers (default: 8)"
    echo "  --no-fp-cost-aware     Floorplanner picks by the SA objective, no cost-model shape refinement"
    echo "  --help                 Show this message"
    echo
    echo "Examples:"
    echo "  $0 48_1_14_4_1600_1600 --tech 7nm"
    echo "  $0 epyc7282 --genetic --tech-nodes 7nm,10nm,14nm --seed 123"
    echo "  $0 epyc7282 --canonical-ga --tech-nodes 7nm,14nm --generations 30 --population 20"
    echo "  $0 ga100 --tech-enum --tech-nodes 7nm,10nm,14nm --max-partitions 3"
    echo "  $0 ga100 --evaluate-partition results/ga100/standard/block_level_netlist.xml.cpart.8"
}

if [ $# -eq 0 ]; then
    show_help
    exit 1
fi
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    show_help
    exit 0
fi

TEST_CASE_NAME="$1"
shift

REACH="0.50"
SEPARATION="0.25"
TECH="7nm"
SEED="42"
GENERATIONS="50"
POPULATION="50"
MAX_PARTITIONS=""
DETAILED_OUTPUT=false
TECH_NODES=""
EVALUATE_PARTITION=""
OUTPUT_DIR=""
EXTRA_OPTS=()
MODE="standard"
MODE_COUNT=0

set_mode() {
    MODE="$1"
    MODE_COUNT=$((MODE_COUNT + 1))
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --help|-h)            show_help; exit 0 ;;
        --reach)              REACH="$2"; shift 2 ;;
        --separation)         SEPARATION="$2"; shift 2 ;;
        --tech)               TECH="$2"; shift 2 ;;
        --seed)               SEED="$2"; shift 2 ;;
        --genetic)            set_mode genetic; shift ;;
        --canonical-ga)       set_mode canonical-ga; shift ;;
        --tech-enum)          set_mode tech-enum; shift ;;
        --evaluate-partition) set_mode evaluate; EVALUATE_PARTITION="$2"; shift 2 ;;
        --max-partitions)     MAX_PARTITIONS="$2"; shift 2 ;;
        --detailed-output)    DETAILED_OUTPUT=true; shift ;;
        --tech-nodes)         TECH_NODES="$2"; shift 2 ;;
        --generations)        GENERATIONS="$2"; shift 2 ;;
        --population)         POPULATION="$2"; shift 2 ;;
        --output-dir)         OUTPUT_DIR="$2"; shift 2 ;;
        --floorplan-aware|--no-floorplan-aware|--fp-eval-candidates|--no-fp-cost-aware)
                              EXTRA_OPTS+=("$1"); shift ;;
        --fp-check-budget|--fp-steps|--fp-perturbs|--fp-workers|--fp-probe-audit|--fp-excursion|--ils-kicks|--ils-parents)
                              EXTRA_OPTS+=("$1" "$2"); shift 2 ;;
        *)
            echo -e "${RED}Error: Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

if [ "$MODE_COUNT" -gt 1 ]; then
    echo -e "${RED}Error: --genetic, --canonical-ga, --tech-enum and --evaluate-partition are mutually exclusive.${NC}"
    exit 1
fi

# Resolve paths relative to this script, not the caller's cwd
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="${SCRIPT_DIR}/build/bin/chipletPart"
TEST_DATA_DIR="${SCRIPT_DIR}/test_data/${TEST_CASE_NAME}"

if [ ! -x "$EXECUTABLE" ]; then
    echo -e "${RED}Error: executable not found: $EXECUTABLE${NC}"
    echo "Build first: mkdir -p build && cd build && cmake .. && make -j"
    exit 1
fi

IO="${TEST_DATA_DIR}/io_definitions.xml"
LAYER="${TEST_DATA_DIR}/layer_definitions.xml"
WAFER="${TEST_DATA_DIR}/wafer_process_definitions.xml"
ASSEMBLY="${TEST_DATA_DIR}/assembly_process_definitions.xml"
TEST="${TEST_DATA_DIR}/test_definitions.xml"
NETLIST="${TEST_DATA_DIR}/block_level_netlist.xml"
BLOCKS="${TEST_DATA_DIR}/block_definitions.txt"

for file in "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}Error: File not found: $file${NC}"
        echo "Please ensure the test data exists in ${TEST_DATA_DIR}."
        exit 1
    fi
done

if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR="${SCRIPT_DIR}/results/${TEST_CASE_NAME}/${MODE}"
fi
mkdir -p "$OUTPUT_DIR"

DESIGN_ARGS=("$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS" "$REACH" "$SEPARATION")
# (${arr[@]+"${arr[@]}"} keeps bash 3 + `set -u` happy when the array is empty)
COMMON_OPTS=(--seed "$SEED" --output-dir "$OUTPUT_DIR" ${EXTRA_OPTS[@]+"${EXTRA_OPTS[@]}"})
CMD=()

echo -e "${CYAN}====================================================================${NC}"
echo -e "${CYAN}  ChipletPart  case=${TEST_CASE_NAME}  mode=${MODE}${NC}"
echo -e "${CYAN}====================================================================${NC}"
echo -e "${BLUE}Reach: ${REACH}, Separation: ${SEPARATION}, Seed: ${SEED}${NC}"
echo -e "${BLUE}Output directory: ${OUTPUT_DIR}${NC}"

case "$MODE" in
    evaluate)
        if [ ! -f "$EVALUATE_PARTITION" ]; then
            echo -e "${RED}Error: Partition file not found: $EVALUATE_PARTITION${NC}"
            exit 1
        fi
        echo -e "${BLUE}Partition file: ${EVALUATE_PARTITION}, Tech: ${TECH}${NC}"
        CMD=("$EXECUTABLE" "$EVALUATE_PARTITION" "${DESIGN_ARGS[@]}" "$TECH" "${COMMON_OPTS[@]}")
        ;;
    tech-enum)
        CMD=("$EXECUTABLE" "${DESIGN_ARGS[@]}" --tech-enum "${COMMON_OPTS[@]}")
        [ -n "$TECH_NODES" ] && CMD+=(--tech-nodes "$TECH_NODES")
        [ -n "$MAX_PARTITIONS" ] && CMD+=(--max-partitions "$MAX_PARTITIONS")
        [ "$DETAILED_OUTPUT" = true ] && CMD+=(--detailed-output)
        echo -e "${BLUE}Tech nodes: ${TECH_NODES:-(default)}, Max partitions: ${MAX_PARTITIONS:-(default)}${NC}"
        ;;
    canonical-ga)
        CMD=("$EXECUTABLE" "${DESIGN_ARGS[@]}" --canonical-ga
             --generations "$GENERATIONS" --population "$POPULATION" "${COMMON_OPTS[@]}")
        [ -n "$TECH_NODES" ] && CMD+=(--tech-nodes "$TECH_NODES")
        [ -n "$MAX_PARTITIONS" ] && CMD+=(--max-partitions "$MAX_PARTITIONS")
        echo -e "${BLUE}Tech nodes: ${TECH_NODES:-(default)}, Generations: ${GENERATIONS}, Population: ${POPULATION}${NC}"
        ;;
    genetic)
        CMD=("$EXECUTABLE" "${DESIGN_ARGS[@]}" --genetic-tech-part
             --generations "$GENERATIONS" --population "$POPULATION" "${COMMON_OPTS[@]}")
        [ -n "$TECH_NODES" ] && CMD+=(--tech-nodes "$TECH_NODES")
        [ -n "$MAX_PARTITIONS" ] && CMD+=(--max-partitions "$MAX_PARTITIONS")
        echo -e "${BLUE}Tech nodes: ${TECH_NODES:-(default)}, Generations: ${GENERATIONS}, Population: ${POPULATION}${NC}"
        ;;
    standard)
        echo -e "${BLUE}Tech node: ${TECH}${NC}"
        CMD=("$EXECUTABLE" "${DESIGN_ARGS[@]}" "$TECH" "${COMMON_OPTS[@]}")
        ;;
esac

echo -e "${YELLOW}\$ ${CMD[*]}${NC}"
echo
"${CMD[@]}" 2>&1 | tee "${OUTPUT_DIR}/run.log"
exit_code=${PIPESTATUS[0]}

echo
if [ "$exit_code" -eq 0 ]; then
    echo -e "${GREEN}Run completed successfully. Files in ${OUTPUT_DIR}:${NC}"
    ls -la "$OUTPUT_DIR"
else
    echo -e "${RED}Run failed with exit code $exit_code (log: ${OUTPUT_DIR}/run.log)${NC}"
fi
exit "$exit_code"
