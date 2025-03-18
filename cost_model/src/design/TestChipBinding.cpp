#include "design/Test.hpp"
#include "design/Chip.hpp"

namespace design {

// Function to initialize Test with Chip functions - empty implementation for compatibility
void InitializeTestWithChipFunctions(Test& test) {
    // Empty implementation - this version is for compatibility
}

// Implementation of InitializeTestWithChipFunctions that used to bind a Test object to a Chip
// Now just a stub for compatibility
void InitializeTestWithChipFunctions(Test& test, const std::shared_ptr<Chip>& chip) {
    // Function pointers are no longer needed as Test methods now take direct parameters
    // This function is kept as a stub for backwards compatibility
}

} // namespace design 