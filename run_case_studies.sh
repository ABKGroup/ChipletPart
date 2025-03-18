#!/bin/bash

# Set colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Function to display help/usage information
show_help() {
    echo -e "${BLUE}Usage: $0 [options]${NC}"
    echo
    echo "Options:"
    echo "  --seed <value>        Specify random seed (default: 42)"
    echo "  --skip-existing       Skip directories where results already exist"
    echo "  --help                Display this help message"
    echo
    echo "This script runs the chipletPart evaluator on all case study directories"
    echo "in the ChipletPartCaseStudies folder."
}

# Default values
DEFAULT_SEED="42"
SKIP_EXISTING=false

# Parse command line arguments
while [ "$#" -gt 0 ]; do
    case "$1" in
        --help)
            show_help
            exit 0
            ;;
        --seed)
            DEFAULT_SEED="$2"
            shift 2
            ;;
        --skip-existing)
            SKIP_EXISTING=true
            shift
            ;;
        *)
            echo -e "${RED}Error: Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Define the base directory where the executable and case studies are located
BASE_DIR="$(pwd)"
BUILD_DIR="${BASE_DIR}/build"
EXECUTABLE="${BUILD_DIR}/bin/chipletPart"
CASE_STUDIES_DIR="${BASE_DIR}/ChipletPartCaseStudies"
RESULTS_DIR="${BASE_DIR}/results/case_studies"

# Create results directory if it doesn't exist
mkdir -p "$RESULTS_DIR"

# Check if case studies directory exists
if [ ! -d "$CASE_STUDIES_DIR" ]; then
    echo -e "${RED}Error: Case studies directory not found: $CASE_STUDIES_DIR${NC}"
    exit 1
fi

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Error: Executable not found: $EXECUTABLE${NC}"
    echo -e "${YELLOW}Make sure you have built the project with cmake${NC}"
    exit 1
fi

# Find all case study directories
CASE_STUDY_DIRS=$(find "$CASE_STUDIES_DIR" -mindepth 1 -maxdepth 1 -type d | sort)

# Count total number of case studies
TOTAL_CASES=$(echo "$CASE_STUDY_DIRS" | wc -l)
CURRENT_CASE=0
SUCCESSFUL_CASES=0
FAILED_CASES=0
SKIPPED_CASES=0

echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo -e "${CYAN}${BOLD}      Running ChipletPart on all Case Study Directories   ${NC}"
echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo -e "${BLUE}Case studies directory: ${YELLOW}$CASE_STUDIES_DIR${NC}"
echo -e "${BLUE}Results directory: ${YELLOW}$RESULTS_DIR${NC}"
echo -e "${BLUE}Total case studies: ${YELLOW}$TOTAL_CASES${NC}"
echo -e "${BLUE}Seed: ${YELLOW}$DEFAULT_SEED${NC}"
echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo

# Process each case study directory
for CASE_STUDY_DIR in $CASE_STUDY_DIRS; do
    # Get the directory name
    CASE_NAME=$(basename "$CASE_STUDY_DIR")
    CURRENT_CASE=$((CURRENT_CASE + 1))
    
    # Define result file path for this case
    RESULT_FILE="${RESULTS_DIR}/${CASE_NAME}.txt"
    
    # Check if result already exists and skip if requested
    if [ "$SKIP_EXISTING" = true ] && [ -f "$RESULT_FILE" ]; then
        echo -e "${YELLOW}Skipping existing case [${CURRENT_CASE}/${TOTAL_CASES}]: ${CASE_NAME}${NC}"
        SKIPPED_CASES=$((SKIPPED_CASES + 1))
        continue
    fi
    
    echo -e "${CYAN}${BOLD}Processing case [${CURRENT_CASE}/${TOTAL_CASES}]: ${CASE_NAME}${NC}"
    
    # Define file paths for the input files in this case study
    IO="${CASE_STUDY_DIR}/io_definitions.xml"
    LAYER="${CASE_STUDY_DIR}/layer_definitions.xml"
    WAFER="${CASE_STUDY_DIR}/wafer_process_definitions.xml"
    ASSEMBLY="${CASE_STUDY_DIR}/assembly_process_definitions.xml"
    TEST="${CASE_STUDY_DIR}/test_definitions.xml"
    
    # Some case studies have different naming conventions, try to handle them
    NETLIST=$(find "$CASE_STUDY_DIR" -name "block_level_netlist*.xml" | head -1)
    BLOCKS=$(find "$CASE_STUDY_DIR" -name "block_definitions*.txt" | head -1)
    
    if [ -z "$NETLIST" ]; then
        echo -e "${RED}Error: Could not find netlist file in $CASE_STUDY_DIR${NC}"
        FAILED_CASES=$((FAILED_CASES + 1))
        continue
    fi
    
    if [ -z "$BLOCKS" ]; then
        echo -e "${RED}Error: Could not find blocks file in $CASE_STUDY_DIR${NC}"
        FAILED_CASES=$((FAILED_CASES + 1))
        continue
    fi
    
    # Extract reach value and separation from the case name if possible
    REACH="0.50"  # Default
    SEPARATION="0.25"  # Default
    TECH="7nm"  # Default
    
    # If case name contains "reach", extract the reach value
    if [[ "$CASE_NAME" =~ reach_([0-9]+) ]]; then
        REACH="0.$(printf "%02d" "${BASH_REMATCH[1]}")"
        echo -e "${BLUE}Extracted reach value: ${YELLOW}$REACH${NC}"
    fi
    
    # If case name contains specific IO type, adjust separation
    if [[ "$CASE_NAME" =~ IO_parallel_10um ]]; then
        SEPARATION="0.10"
        echo -e "${BLUE}Set separation for parallel IO: ${YELLOW}$SEPARATION${NC}"
    elif [[ "$CASE_NAME" =~ IO_ucie_advanced_55um ]]; then
        SEPARATION="0.55"
        echo -e "${BLUE}Set separation for UCIe advanced: ${YELLOW}$SEPARATION${NC}"
    elif [[ "$CASE_NAME" =~ IO_ucie_standard_110um ]]; then
        SEPARATION="1.10"
        echo -e "${BLUE}Set separation for UCIe standard: ${YELLOW}$SEPARATION${NC}"
    fi
    
    # Check if all required files exist
    FILES_MISSING=false
    for file in "$IO" "$LAYER" "$WAFER" "$ASSEMBLY" "$TEST" "$NETLIST" "$BLOCKS"; do
        if [ ! -f "$file" ]; then
            echo -e "${RED}Error: File not found: $file${NC}"
            FILES_MISSING=true
        fi
    done
    
    if [ "$FILES_MISSING" = true ]; then
        echo -e "${RED}Skipping case due to missing files: $CASE_NAME${NC}"
        FAILED_CASES=$((FAILED_CASES + 1))
        continue
    fi
    
    # Run the executable and capture output
    echo -e "${GREEN}Running evaluation for case: $CASE_NAME${NC}"
    echo -e "${BLUE}Parameters: reach=${YELLOW}$REACH${BLUE}, separation=${YELLOW}$SEPARATION${BLUE}, tech=${YELLOW}$TECH${NC}"
    
    # Create a log file
    {
        echo "========================================================"
        echo "Case Study: $CASE_NAME"
        echo "Executed on: $(date)"
        echo "Parameters:"
        echo "  - Reach: $REACH"
        echo "  - Separation: $SEPARATION" 
        echo "  - Tech: $TECH"
        echo "  - Seed: $DEFAULT_SEED"
        echo "Files:"
        echo "  - IO: $IO"
        echo "  - Layer: $LAYER"
        echo "  - Wafer: $WAFER"
        echo "  - Assembly: $ASSEMBLY"
        echo "  - Test: $TEST"
        echo "  - Netlist: $NETLIST"
        echo "  - Blocks: $BLOCKS"
        echo "========================================================"
        echo
    } > "$RESULT_FILE"
    
    # Run the executable with standard partitioning
    "$EXECUTABLE" \
        "$IO" \
        "$LAYER" \
        "$WAFER" \
        "$ASSEMBLY" \
        "$TEST" \
        "$NETLIST" \
        "$BLOCKS" \
        "$REACH" \
        "$SEPARATION" \
        "$TECH" \
        --seed "$DEFAULT_SEED" >> "$RESULT_FILE" 2>&1
    
    exit_code=$?
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}Case completed successfully!${NC}"
        
        # Extract and display key results
        COST=$(grep -o "Cost of partition is [0-9.]*" "$RESULT_FILE" | awk '{print $NF}')
        PARTITIONS=$(grep -o "Number of partitions is [0-9]*" "$RESULT_FILE" | awk '{print $NF}')
        FEASIBILITY=$(grep -o "Floorplan feasibility is [0-1]" "$RESULT_FILE" | awk '{print $NF}')
        
        echo -e "${BLUE}Results:${NC}"
        echo -e "${BLUE}  - Cost: ${YELLOW}$COST${NC}"
        echo -e "${BLUE}  - Partitions: ${YELLOW}$PARTITIONS${NC}"
        if [ "$FEASIBILITY" -eq 1 ]; then
            echo -e "${BLUE}  - Floorplan feasibility: ${GREEN}YES${NC}"
        else
            echo -e "${BLUE}  - Floorplan feasibility: ${RED}NO${NC}"
        fi
        
        # Copy the partition file if it exists
        if [ ! -z "$PARTITIONS" ] && [ -n "$PARTITIONS" ]; then
            # The partition file is named as per ChipletPart.cpp: netlist_file + ".cpart." + best_result.num_parts
            PARTITION_FILE="${NETLIST}.cpart.${PARTITIONS}"
            
            # Check if partition file exists
            if [ -f "$PARTITION_FILE" ]; then
                # Create a special name for the copied file
                SPECIAL_PARTITION_FILE="${RESULTS_DIR}/${CASE_NAME}_partition_${PARTITIONS}_seed${DEFAULT_SEED}.cpart"
                
                # Copy the file
                cp "$PARTITION_FILE" "$SPECIAL_PARTITION_FILE"
                
                echo -e "${BLUE}Partition file copied to: ${YELLOW}$SPECIAL_PARTITION_FILE${NC}"
                
                # Add information about the partition file to the result file
                {
                    echo ""
                    echo "========================================================"
                    echo "Partition file information:"
                    echo "  - Original location: $PARTITION_FILE"
                    echo "  - Copied to: $SPECIAL_PARTITION_FILE"
                    echo "  - Number of partitions: $PARTITIONS"
                    echo "  - Seed used: $DEFAULT_SEED"
                    echo "========================================================"
                } >> "$RESULT_FILE"
            else
                echo -e "${YELLOW}Warning: Expected partition file not found: $PARTITION_FILE${NC}"
            fi
        fi
        
        SUCCESSFUL_CASES=$((SUCCESSFUL_CASES + 1))
    else
        echo -e "${RED}Case failed with exit code $exit_code${NC}"
        FAILED_CASES=$((FAILED_CASES + 1))
    fi
    
    echo -e "${BLUE}Results saved to: ${YELLOW}$RESULT_FILE${NC}"
    echo
done

# Display summary
echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo -e "${CYAN}${BOLD}                  Summary                               ${NC}"
echo -e "${CYAN}${BOLD}=========================================================${NC}"
echo -e "${BLUE}Total case studies: ${YELLOW}$TOTAL_CASES${NC}"
echo -e "${BLUE}Successful: ${GREEN}$SUCCESSFUL_CASES${NC}"
echo -e "${BLUE}Failed: ${RED}$FAILED_CASES${NC}"
echo -e "${BLUE}Skipped: ${YELLOW}$SKIPPED_CASES${NC}"
echo -e "${BLUE}Results directory: ${YELLOW}$RESULTS_DIR${NC}"
echo -e "${CYAN}${BOLD}=========================================================${NC}" 