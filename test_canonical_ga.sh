#!/bin/bash

# ANSI colors for better output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Test directories
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"
DATA_DIR="${SCRIPT_DIR}/test_data/ga100"

# Print header
echo -e "${CYAN}====================================================================${NC}"
echo -e "${CYAN}            Testing Canonical GA for ChipletPart                    ${NC}"
echo -e "${CYAN}====================================================================${NC}"

# Define files
IO_FILE="${DATA_DIR}/io_definitions.xml"
LAYER_FILE="${DATA_DIR}/layer_definitions.xml"
WAFER_PROCESS_FILE="${DATA_DIR}/wafer_process_definitions.xml"
ASSEMBLY_PROCESS_FILE="${DATA_DIR}/assembly_process_definitions.xml"
TEST_FILE="${DATA_DIR}/test_definitions.xml"
NETLIST_FILE="${DATA_DIR}/block_level_netlist.xml"
BLOCKS_FILE="${DATA_DIR}/block_definitions.txt"

# Check that files exist
for file in "${IO_FILE}" "${LAYER_FILE}" "${WAFER_PROCESS_FILE}" "${ASSEMBLY_PROCESS_FILE}" "${TEST_FILE}" "${NETLIST_FILE}" "${BLOCKS_FILE}"; do
  if [ ! -f "${file}" ]; then
    echo -e "${RED}Error: File not found: ${file}${NC}"
    exit 1
  fi
done

# Parameters
REACH="0.50"
SEPARATION="0.25"
TECH_NODES="7nm,10nm,14nm"
GENERATIONS="3"
POPULATION="3"
SEED="42"

# Create a log directory
LOG_DIR="${SCRIPT_DIR}/logs"
mkdir -p "${LOG_DIR}"
LOG_FILE="${LOG_DIR}/canonical_ga_test_$(date +%Y%m%d_%H%M%S).log"

# Print command for visibility
COMMAND="${BIN_DIR}/chipletPart '${IO_FILE}' '${LAYER_FILE}' '${WAFER_PROCESS_FILE}' '${ASSEMBLY_PROCESS_FILE}' '${TEST_FILE}' '${NETLIST_FILE}' '${BLOCKS_FILE}' ${REACH} ${SEPARATION} --canonical-ga --tech-nodes ${TECH_NODES} --generations ${GENERATIONS} --population ${POPULATION} --seed ${SEED}"

echo -e "${YELLOW}Executing:${NC}"
echo "${COMMAND}"
echo

# Run the command and redirect output to log file
echo "Running with command:" > "${LOG_FILE}"
echo "${COMMAND}" >> "${LOG_FILE}"
echo "---------------" >> "${LOG_FILE}"

eval "${COMMAND}" 2>&1 | tee -a "${LOG_FILE}"
exit_code=${PIPESTATUS[0]}

echo

if [ ${exit_code} -eq 0 ]; then
  echo -e "${GREEN}Test completed successfully (exit code: ${exit_code})${NC}"
else
  echo -e "${RED}Test failed with exit code: ${exit_code}${NC}"
  echo -e "${YELLOW}See log file for details: ${LOG_FILE}${NC}"
fi

echo -e "${CYAN}====================================================================${NC}"
echo -e "${CYAN}                       Test Completed                               ${NC}"
echo -e "${CYAN}====================================================================${NC}"

exit ${exit_code} 
