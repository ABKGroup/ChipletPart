# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.12)

# Project's name
project(PythonEmbeddingExample)

# Find Python 3. Prefer the Python libraries that match the specified version exactly.
find_package(Python3 3.10 EXACT REQUIRED COMPONENTS Interpreter Development)

# Add the executable that we will build from the source file "example.cpp"
add_executable(example example.cpp)

# Include directories for Python
target_include_directories(example PRIVATE ${Python3_INCLUDE_DIRS})

# Link libraries for Python
target_link_libraries(example Python3::Python Python3::Module)

# Additional C++ flags
set_target_properties(example PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

