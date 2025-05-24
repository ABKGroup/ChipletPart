# CMakeLists.txt for cost_model module
# This file is included from the top-level CMakeLists.txt

# Define the cost_model library with evaluator_cpp.cpp
add_library(cost_model 
            ${CMAKE_CURRENT_SOURCE_DIR}/evaluator_cpp.cpp)

# Library dependencies
target_link_libraries(cost_model PRIVATE 
                     ${Python3_LIBRARIES}
                     ${Boost_LIBRARIES}
                     pugixml)

# Link with OpenMP if found
if(OpenMP_CXX_FOUND)
    target_link_libraries(cost_model PRIVATE ${OpenMP_CXX_LIBRARIES})
endif()

# Make sure the include directories are properly set
target_include_directories(cost_model PUBLIC
                          ${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create Design library from source files if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/design")
    file(GLOB DESIGN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/design/*.cpp")
    if(DESIGN_SOURCES)
        add_library(Design ${DESIGN_SOURCES})
        target_link_libraries(Design pugixml)
        target_link_libraries(cost_model PRIVATE Design)
        
        # Link with OpenMP if found
        if(OpenMP_CXX_FOUND)
            target_link_libraries(Design ${OpenMP_CXX_LIBRARIES})
        endif()
    endif()
endif()

# Conditionally create ConstructChip library
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ConstructChip.cpp")
    add_library(ConstructChip ${CMAKE_CURRENT_SOURCE_DIR}/src/ConstructChip.cpp)
    if(TARGET Design)
        target_link_libraries(ConstructChip pugixml Design)
    else()
        target_link_libraries(ConstructChip pugixml)
    endif()
    target_link_libraries(cost_model PRIVATE ConstructChip)
endif()

# Conditionally create ReadDesignFromFile library
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadDesignFromFile.cpp")
    add_library(ReadDesignFromFile ${CMAKE_CURRENT_SOURCE_DIR}/src/ReadDesignFromFile.cpp)
    if(TARGET Design)
        target_link_libraries(ReadDesignFromFile pugixml Design)
    else()
        target_link_libraries(ReadDesignFromFile pugixml)
    endif()
    target_link_libraries(cost_model PRIVATE ReadDesignFromFile)
endif()

# Conditionally create example executables and tests if requested
option(BUILD_COST_MODEL_EXAMPLES "Build cost model examples" OFF)
if(BUILD_COST_MODEL_EXAMPLES)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
        # Add example executables if example directory exists
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/construction_example.cpp" AND TARGET ConstructChip)
            add_executable(construction_example ${CMAKE_CURRENT_SOURCE_DIR}/examples/construction_example.cpp)
            target_link_libraries(construction_example ConstructChip)
            if(OpenMP_CXX_FOUND)
                target_link_libraries(construction_example ${OpenMP_CXX_LIBRARIES})
            endif()
        endif()
        
        # More examples...
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/design/wafer_process_example.cpp" AND TARGET Design)
            add_executable(wafer_process_example ${CMAKE_CURRENT_SOURCE_DIR}/examples/design/wafer_process_example.cpp)
            target_link_libraries(wafer_process_example Design)
            if(OpenMP_CXX_FOUND)
                target_link_libraries(wafer_process_example ${OpenMP_CXX_LIBRARIES})
            endif()
        endif()
        
        # Add test_api_cpp executable if it exists
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_api_cpp.cpp")
            add_executable(test_api_cpp ${CMAKE_CURRENT_SOURCE_DIR}/test_api_cpp.cpp)
            target_link_libraries(test_api_cpp cost_model)
            if(OpenMP_CXX_FOUND)
                target_link_libraries(test_api_cpp ${OpenMP_CXX_LIBRARIES})
            endif()
        endif()
    endif()
endif()

# Add tests if they exist and if testing is enabled
option(BUILD_COST_MODEL_TESTS "Build cost model tests" OFF)
if(BUILD_COST_MODEL_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()

# Export the cost_model target for use by other components
set_target_properties(cost_model PROPERTIES POSITION_INDEPENDENT_CODE ON) 
