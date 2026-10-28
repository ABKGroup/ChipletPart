#!/bin/bash
# Evaluate an existing partition of a test case with the ChipletPart cost model.
#
#   ./run_evaluator.sh <test_case_name> --partition <file> [options]
#
# The partition file has one chiplet id per line, in block order (e.g. a
# *.cpart.N file written by run_chiplet_test.sh).
set -uo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

show_help() {
    echo -e "${BLUE}Usage: $0 <test_case_name> --partition <file> [options]${NC}"
    echo
    echo "Options:"
    echo "  --partition <file>    Partition file to evaluate (required)"
    echo "  --reach <value>       Reach in mm (default: 0.50)"
    echo "  --separation <value>  Separation in mm (default: 0.25)"
    echo "  --tech <node>         Tech node for every chiplet (default: 7nm)"
    echo "  --output-dir <dir>    Where result files go (default: results/<test_case_name>)"
    echo "  --help                Display this help message"
    echo
    echo "Example:"
    echo "  $0 ga100 --partition results/ga100/block_level_netlist.xml.cpart.8 --tech 7nm"
}

if [ $# -eq 0 ]; then
    show_help
    exit 1
fi

TEST_CASE_NAME="$1"
shift

REACH="0.50"
SEPARATION="0.25"
TECH="7nm"
PARTITION=""
OUTPUT_DIR=""

while [ "$#" -gt 0 ]; do
    case "$1" in
        --help) show_help; exit 0 ;;
        --partition) PARTITION="$2"; shift 2 ;;
        --reach) REACH="$2"; shift 2 ;;
        --separation) SEPARATION="$2"; shift 2 ;;
        --tech) TECH="$2"; shift 2 ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        *)
            echo -e "${RED}Error: Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

if [ -z "$PARTITION" ]; then
    echo -e "${RED}Error: --partition <file> is required${NC}"
    show_help
    exit 1
fi

BASE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
EXECUTABLE="${BASE_DIR}/build/bin/chipletPart"
TEST_DATA_DIR="${BASE_DIR}/test_data/${TEST_CASE_NAME}"
if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR="${BASE_DIR}/results/${TEST_CASE_NAME}"
fi
mkdir -p "$OUTPUT_DIR"

IO="${TEST_DATA_DIR}/io_definitions.xml"
LAYER="${TEST_DATA_DIR}/layer_definitions.xml"
WAFER="${TEST_DATA_DIR}/wafer_process_definitions.xml"
ASSEMBLY="${TEST_DATA_DIR}/assembly_process_definitions.xml"
TEST="${TEST_DATA_DIR}/test_definitions.xml"
NETLIST="${TEST_DATA_DIR}/block_level_netlist.xml"
BLOCKS="${TEST_DATA_DIR}/block_definitions.txt"

for file in "$PARTITION" "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}Error: File not found: $file${NC}"
        exit 1
    fi
done

if [ ! -x "$EXECUTABLE" ]; then
    echo -e "${RED}Error: $EXECUTABLE not found${NC}"
    echo -e "${YELLOW}Build the project first (mkdir build && cd build && cmake .. && make)${NC}"
    exit 1
fi

echo -e "${GREEN}Evaluating partition:${NC}"
echo -e "${BLUE}Partition file: ${YELLOW}$PARTITION${NC}"
echo -e "${BLUE}Test case:      ${YELLOW}$TEST_CASE_NAME${NC}"
echo -e "${BLUE}Reach:          ${YELLOW}$REACH${NC}"
echo -e "${BLUE}Separation:     ${YELLOW}$SEPARATION${NC}"
echo -e "${BLUE}Technology:     ${YELLOW}$TECH${NC}"
echo

"$EXECUTABLE" \
    "$PARTITION" \
    "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS" \
    "$REACH" "$SEPARATION" "$TECH" \
    --output-dir "$OUTPUT_DIR"
exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo -e "${GREEN}Evaluation completed successfully${NC}"
else
    echo -e "${RED}Evaluation failed with exit code $exit_code${NC}"
fi
exit $exit_code
