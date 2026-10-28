#!/bin/bash
# Run the canonical GA (--canonical-ga) on one test case.
set -euo pipefail

# Default parameters
TECH_NODES="7nm,10nm,14nm"
GENERATIONS=50
POPULATION=50
SEED=42
REACH=0.5
SEPARATION=0.25
TEST_CASE="ga100"
OUTPUT_DIR=""

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    --tech-nodes)
      TECH_NODES="$2"
      shift 2
      ;;
    --generations)
      GENERATIONS="$2"
      shift 2
      ;;
    --population)
      POPULATION="$2"
      shift 2
      ;;
    --seed)
      SEED="$2"
      shift 2
      ;;
    --reach)
      REACH="$2"
      shift 2
      ;;
    --separation)
      SEPARATION="$2"
      shift 2
      ;;
    --test-case)
      TEST_CASE="$2"
      shift 2
      ;;
    --output-dir)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    --help)
      echo "Usage: $0 [options]"
      echo ""
      echo "Options:"
      echo "  --tech-nodes <list>     Comma-separated list of technology nodes (default: 7nm,10nm,14nm)"
      echo "  --generations <num>     Number of generations (default: 50)"
      echo "  --population <num>      Population size (default: 50)"
      echo "  --seed <num>            Random seed for reproducibility (default: 42)"
      echo "  --reach <num>           Reach parameter in mm (default: 0.5)"
      echo "  --separation <num>      Separation parameter in mm (default: 0.25)"
      echo "  --test-case <name>      Directory under test_data/ (default: ga100)"
      echo "  --output-dir <dir>      Where result files go (default: results/canonical_ga/<test-case>)"
      echo "  --help                  Display this help message"
      exit 0
      ;;
    *)
      echo "Unknown option: $key"
      echo "Use --help for usage information"
      exit 1
      ;;
  esac
done

# ANSI colors for better output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Print header
echo -e "${CYAN}====================================================================${NC}"
echo -e "${CYAN}                 Canonical GA for ChipletPart                       ${NC}"
echo -e "${CYAN}====================================================================${NC}"

echo -e "${GREEN}Using full Canonical Genetic Algorithm implementation${NC}"
echo

# Define directories
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"
DATA_DIR="${SCRIPT_DIR}/test_data"

# Check that required directories and files exist
if [ ! -d "${BIN_DIR}" ]; then
  echo -e "${RED}Error: Binary directory not found: ${BIN_DIR}${NC}"
  echo -e "${YELLOW}Please build the project first.${NC}"
  exit 1
fi

if [ ! -f "${BIN_DIR}/chipletPart" ]; then
  echo -e "${RED}Error: ChipletPart executable not found: ${BIN_DIR}/chipletPart${NC}"
  echo -e "${YELLOW}Please build the project first.${NC}"
  exit 1
fi

# Locate test files
CASE_DIR="${DATA_DIR}/${TEST_CASE}"
IO_FILE="${CASE_DIR}/io_definitions.xml"
LAYER_FILE="${CASE_DIR}/layer_definitions.xml"
WAFER_PROCESS_FILE="${CASE_DIR}/wafer_process_definitions.xml"
ASSEMBLY_PROCESS_FILE="${CASE_DIR}/assembly_process_definitions.xml"
TEST_FILE="${CASE_DIR}/test_definitions.xml"
NETLIST_FILE="${CASE_DIR}/block_level_netlist.xml"
BLOCKS_FILE="${CASE_DIR}/block_definitions.txt"

for file in "${IO_FILE}" "${LAYER_FILE}" "${WAFER_PROCESS_FILE}" "${ASSEMBLY_PROCESS_FILE}" "${TEST_FILE}" "${NETLIST_FILE}" "${BLOCKS_FILE}"; do
  if [ ! -f "${file}" ]; then
    echo -e "${RED}Error: Required file not found: ${file}${NC}"
    exit 1
  fi
done

if [ -z "${OUTPUT_DIR}" ]; then
  OUTPUT_DIR="${SCRIPT_DIR}/results/canonical_ga/${TEST_CASE}"
fi
mkdir -p "${OUTPUT_DIR}"

# Print the configuration
echo -e "${BLUE}Running Canonical GA with the following parameters:${NC}"
echo -e "  Tech Nodes:      ${TECH_NODES}"
echo -e "  Generations:     ${GENERATIONS}"
echo -e "  Population Size: ${POPULATION}"
echo -e "  Random Seed:     ${SEED}"
echo -e "  Reach:           ${REACH}"
echo -e "  Separation:      ${SEPARATION}"
echo -e "  Test Case:       ${TEST_CASE}"
echo -e "  Output Dir:      ${OUTPUT_DIR}"
echo

if "${BIN_DIR}/chipletPart" "${IO_FILE}" "${LAYER_FILE}" "${WAFER_PROCESS_FILE}" "${ASSEMBLY_PROCESS_FILE}" \
              "${TEST_FILE}" "${NETLIST_FILE}" "${BLOCKS_FILE}" \
              "${REACH}" "${SEPARATION}" --canonical-ga \
              --tech-nodes "${TECH_NODES}" \
              --generations "${GENERATIONS}" \
              --population "${POPULATION}" \
              --seed "${SEED}" \
              --output-dir "${OUTPUT_DIR}"; then
  echo -e "${GREEN}Canonical GA completed successfully!${NC}"
  echo -e "${BLUE}Generated output files:${NC}"
  ls -la "${OUTPUT_DIR}"
else
  echo -e "${RED}Canonical GA failed!${NC}"
  exit 1
fi

echo -e "${CYAN}====================================================================${NC}"
echo -e "${CYAN}                       Execution Complete                           ${NC}"
echo -e "${CYAN}====================================================================${NC}" 