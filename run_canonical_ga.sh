#!/bin/bash

# Default parameters
TECH_NODES="7nm,14nm,28nm"
GENERATIONS=50
POPULATION=50
SEED=42
REACH=5.0
SEPARATION=10.0
OUTPUT_PREFIX="canonical_ga_result"

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
    --output-prefix)
      OUTPUT_PREFIX="$2"
      shift 2
      ;;
    --help)
      echo "Usage: $0 [options]"
      echo ""
      echo "Options:"
      echo "  --tech-nodes <list>     Comma-separated list of technology nodes (default: 7nm,14nm,28nm)"
      echo "  --generations <num>     Number of generations (default: 50)"
      echo "  --population <num>      Population size (default: 50)"
      echo "  --seed <num>            Random seed for reproducibility (default: 42)"
      echo "  --reach <num>           Reach parameter (default: 5.0)"
      echo "  --separation <num>      Separation parameter (default: 10.0)"
      echo "  --output-prefix <n>  Prefix for output files (default: canonical_ga_result)"
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
IO_FILE="${DATA_DIR}/test_io.xml"
LAYER_FILE="${DATA_DIR}/test_layer.xml"
WAFER_PROCESS_FILE="${DATA_DIR}/test_wafer_process.xml"
ASSEMBLY_PROCESS_FILE="${DATA_DIR}/test_assembly_process.xml"
TEST_FILE="${DATA_DIR}/test_test.xml"
NETLIST_FILE="${DATA_DIR}/test_netlist.xml"
BLOCKS_FILE="${DATA_DIR}/test_blocks.xml"

# Check that all test files exist
for file in "${IO_FILE}" "${LAYER_FILE}" "${WAFER_PROCESS_FILE}" "${ASSEMBLY_PROCESS_FILE}" "${TEST_FILE}" "${NETLIST_FILE}" "${BLOCKS_FILE}"; do
  if [ ! -f "${file}" ]; then
    echo -e "${YELLOW}Warning: Test file not found: ${file}${NC}"
    echo -e "${YELLOW}Using ga100 test files instead.${NC}"
    # Try using ga100 test files
    DATA_DIR="${SCRIPT_DIR}/test_data/ga100"
    IO_FILE="${DATA_DIR}/io_definitions.xml"
    LAYER_FILE="${DATA_DIR}/layer_definitions.xml"
    WAFER_PROCESS_FILE="${DATA_DIR}/wafer_process_definitions.xml"
    ASSEMBLY_PROCESS_FILE="${DATA_DIR}/assembly_process_definitions.xml"
    TEST_FILE="${DATA_DIR}/test_definitions.xml"
    NETLIST_FILE="${DATA_DIR}/block_level_netlist.xml"
    BLOCKS_FILE="${DATA_DIR}/block_definitions.txt"
    break
  fi
done

# Check again that all files exist
for file in "${IO_FILE}" "${LAYER_FILE}" "${WAFER_PROCESS_FILE}" "${ASSEMBLY_PROCESS_FILE}" "${TEST_FILE}" "${NETLIST_FILE}" "${BLOCKS_FILE}"; do
  if [ ! -f "${file}" ]; then
    echo -e "${RED}Error: Required file not found: ${file}${NC}"
    echo -e "${YELLOW}Please create sample test files first.${NC}"
    exit 1
  fi
done

# Print the configuration
echo -e "${BLUE}Running Canonical GA with the following parameters:${NC}"
echo -e "  Tech Nodes:      ${TECH_NODES}"
echo -e "  Generations:     ${GENERATIONS}"
echo -e "  Population Size: ${POPULATION}"
echo -e "  Random Seed:     ${SEED}"
echo -e "  Reach:           ${REACH}"
echo -e "  Separation:      ${SEPARATION}"
echo -e "  Output Prefix:   ${OUTPUT_PREFIX}"
echo

# Execute the ChipletPart binary with canonical GA
cd "${BIN_DIR}"

# Run with smaller values for testing
GENERATIONS=3
POPULATION=3

./chipletPart "${IO_FILE}" "${LAYER_FILE}" "${WAFER_PROCESS_FILE}" "${ASSEMBLY_PROCESS_FILE}" \
              "${TEST_FILE}" "${NETLIST_FILE}" "${BLOCKS_FILE}" \
              ${REACH} ${SEPARATION} --canonical-ga \
              --tech-nodes "${TECH_NODES}" \
              --generations ${GENERATIONS} \
              --population ${POPULATION} \
              --seed ${SEED}

# Check the exit status
if [ $? -eq 0 ]; then
  echo -e "${GREEN}Canonical GA completed successfully!${NC}"
  
  # List output files
  echo -e "${BLUE}Generated output files:${NC}"
  ls -la canonical_ga_result*
else
  echo -e "${RED}Canonical GA failed!${NC}"
  exit 1
fi

echo -e "${CYAN}====================================================================${NC}"
echo -e "${CYAN}                       Execution Complete                           ${NC}"
echo -e "${CYAN}====================================================================${NC}" 