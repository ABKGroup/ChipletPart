#!/bin/bash

# Set colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo -e "${RED}Usage: $0 <source_floorplet_file> <output_file>${NC}"
    echo -e "Example: $0 test_data/48_1_14_4_1600_1600/floorplet/block_definitions.txt.floorplet_part.min_num_chiplets_10.max_num_chiplets_10 test_data/48_1_14_4_1600_1600/manual.part"
    exit 1
fi

SOURCE_FILE="$1"
OUTPUT_FILE="$2"

# Check if source file exists
if [ ! -f "$SOURCE_FILE" ]; then
    echo -e "${RED}Error: Source file not found: $SOURCE_FILE${NC}"
    exit 1
fi

# Create output directory if it doesn't exist
OUTPUT_DIR=$(dirname "$OUTPUT_FILE")
mkdir -p "$OUTPUT_DIR"

# Convert the file
echo -e "${GREEN}Converting floorplet partition file to evaluator format...${NC}"

# Skip the header line and extract only the chiplet_id column
tail -n +2 "$SOURCE_FILE" | awk '{print $2}' > "$OUTPUT_FILE"

# Check if conversion was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Conversion completed. Output written to $OUTPUT_FILE${NC}"
    echo -e "${BLUE}Number of blocks processed: $(wc -l < $OUTPUT_FILE)${NC}"
else
    echo -e "${RED}Conversion failed${NC}"
    exit 1
fi 