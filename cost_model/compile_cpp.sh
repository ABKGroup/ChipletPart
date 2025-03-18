#!/bin/bash

set -e  # Exit immediately if a command exits with a non-zero status

# Create build directory if it doesn't exist
mkdir -p build_cpp

# Set include paths
INCLUDES="-I./include -I./build/_deps/pugixml-src/src"

# Compile evaluator_cpp.cpp to an object file
g++ -std=c++17 -c evaluator_cpp.cpp -o build_cpp/evaluator_cpp.o ${INCLUDES}

# Compile test_api_cpp.cpp to an object file
g++ -std=c++17 -c test_api_cpp.cpp -o build_cpp/test_api_cpp.o ${INCLUDES}

# Link the final executable
g++ build_cpp/evaluator_cpp.o build_cpp/test_api_cpp.o -o build_cpp/test_api_cpp -Lbuild/lib -lDesign -lReadDesignFromFile -lConstructChip -lpugixml

echo "Compilation completed successfully. Executable is at build_cpp/test_api_cpp" 