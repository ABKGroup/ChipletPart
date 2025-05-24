# ChipletPart C++ Library

This is a C++ implementation of the ChipletPart library, ported from Python to C++.

## Features

- Extract technology nodes from complex string formats
- Create XML element trees for chip definitions
- Combine block adjacency matrices
- Increment netlists for specific partitions

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.14 or newer
- Git (for fetching dependencies)

## Dependencies

- [pugixml](https://github.com/zeux/pugixml) - Used for XML processing

## Building the Library

### Clone the repository

```bash
git clone https://github.com/your-username/chipletpart.git
cd chipletpart
```

### Build with CMake

```bash
# Create a build directory
mkdir build
cd build

# Configure CMake
cmake ..

# Build the library
cmake --build .
```

The CMake build system will automatically fetch and build pugixml if it's not already installed on your system.

### Run the example

After building, you can run the example program:

```bash
# From the build directory
./bin/construction_example
```

## Usage

```cpp
#include <chipletpart/ConstructChip.hpp>

// Extract a technology node
std::string tech_node = construct_chip::ExtractTechNode("BT_TSMC_7nm");
// Result: "7nm"

// Create an element tree
pugi::xml_document doc = construct_chip::CreateElementTree(
    subtree_tech_nodes,
    subtree_aspect_ratios,
    subtree_x_locations,
    subtree_y_locations,
    subtree_power,
    subtree_core_area,
    num_subtrees
);

// Increment a netlist
auto updated_connectivity = construct_chip::IncrementNetlist(
    connectivity,
    bandwidth_change_for_slope,
    partition_id
);
```

## License

[MIT License](LICENSE) 