#ifndef EVALUATOR_CPP_H
#define EVALUATOR_CPP_H

#include <string>
#include <vector>
#include <memory>
#include <map>

#include "include/design/Design.hpp"
#include "include/design/Chip.hpp"
#include "include/design/Layer.hpp"
#include "include/design/IO.hpp"
#include "include/design/WaferProcess.hpp"
#include "include/design/Assembly.hpp"
#include "include/design/Test.hpp"
#include "include/ReadDesignFromFile.hpp"
#include "include/ConstructChip.hpp"

// Define the block structure to match the original harness
struct block {
    std::string name;
    float area;
    float power;
    std::string tech;
    bool is_memory;
};

// Structure to hold all library definitions
struct LibraryDicts {
    std::vector<design::WaferProcess> wafer_processes;
    std::vector<design::IO> ios;
    std::vector<design::Layer> layers;
    std::vector<design::Assembly> assemblies;
    std::vector<design::Test> tests;
    std::map<std::string, std::vector<std::vector<int>>> global_adjacency_matrix;
    std::map<std::string, std::vector<std::vector<double>>> average_bandwidth_utilization;
    std::vector<std::string> block_names;
};

// Helper function for scaling areas between technology nodes
float area_scaling_factor(std::string initial_tech_node, std::string actual_tech_node, bool is_memory);

// Helper function for scaling power between technology nodes
float power_scaling_factor(std::string initial_tech_node, std::string actual_tech_node);

// Read library definitions from files
LibraryDicts* readLibraries(std::string io_file, std::string layer_file, std::string wafer_process_file, 
                            std::string assembly_process_file, std::string test_file, std::string netlist_file);

// Read block definitions from file
std::vector<block> readBlocks(std::string blocks_file);

// Initialize database and return library dictionaries
LibraryDicts* init(std::string io_file, std::string layer_file, std::string wafer_process_file, 
                  std::string assembly_process_file, std::string test_file, std::string netlist_file, 
                  std::string blocks_file);

// Free resources associated with library dictionaries
int destroyDatabase(LibraryDicts* libraryDicts);

// Helper function to get number of partitions
int getNumPartitions(const std::vector<int>& partitionIDs);

// Helper function to group blocks by partition
std::vector<std::vector<int>> getPartitionVector(const std::vector<int>& partitionIDs);

// Calculate areas for each partition
float getAreas(std::vector<float>& chiplet_areas, const std::vector<std::vector<int>>& partitionVector, 
              const std::vector<block>& blocks, const std::vector<std::string>& tech_array, int numPartitions);

// Calculate power for each partition
float getPowers(std::vector<float>& chiplet_powers, const std::vector<std::vector<int>>& partitionVector, 
               const std::vector<block>& blocks, const std::vector<std::string>& tech_array, int numPartitions);

// Get block chiplet names for each partition
std::vector<std::string> getBlockChipletNames(int numPartitions);

// Get block names from the blocks vector
std::vector<std::string> getBlockNames(const std::vector<block>& blocks);

// Build a chip model based on partition IDs
std::shared_ptr<design::Chip> buildModel(const std::vector<int>& partitionIDs, 
                                         const std::vector<std::string>& tech_array, 
                                         const std::vector<float>& aspect_ratios, 
                                         const std::vector<float>& x_locations, 
                                         const std::vector<float>& y_locations, 
                                         LibraryDicts* libraryDicts, 
                                         const std::vector<block>& blocks,
                                         bool approx_state = false);

// Free resources associated with a chip model
int destroyModel(std::shared_ptr<design::Chip> model);

// Calculate cost from scratch for a given partition configuration
float getCostFromScratch(const std::vector<int>& partitionIds, 
                         const std::vector<std::string>& tech_array, 
                         const std::vector<float>& aspect_ratios, 
                         const std::vector<float>& x_locations, 
                         const std::vector<float>& y_locations, 
                         LibraryDicts* libraryDicts, 
                         const std::vector<block>& blocks, 
                         float cost_coefficient, 
                         float power_coefficient,
                         bool approx_state = false);

// Calculate cost and slopes for gradient-based optimization
float getCostAndSlopes(const std::vector<int>& partitionIds, 
                       const std::vector<std::string>& tech_array, 
                       const std::vector<float>& aspect_ratios, 
                       const std::vector<float>& x_locations, 
                       const std::vector<float>& y_locations, 
                       LibraryDicts* libraryDicts, 
                       const std::vector<block>& blocks, 
                       std::vector<float>& costAreaSlopes, 
                       std::vector<float>& powerAreaSlopes, 
                       std::vector<float>& costBandwidthSlopes, 
                       std::vector<float>& powerBandwidthSlopes, 
                       float& costConfidenceInterval, 
                       float& powerConfidenceInterval, 
                       float cost_coefficient, 
                       float power_coefficient);

// Calculate costs for moving each block to each partition
std::vector<std::vector<float>> getCostIncremental(const std::vector<int>& basePartitionIds, 
                                                 const std::vector<std::string>& tech_array, 
                                                 const std::vector<float>& aspect_ratios, 
                                                 const std::vector<float>& x_locations, 
                                                 const std::vector<float>& y_locations, 
                                                 const int numPartitions, 
                                                 LibraryDicts* libraryDicts, 
                                                 const std::vector<block>& blocks, 
                                                 float cost_coefficient, 
                                                 float power_coefficient);

// Calculate cost for moving a single block between partitions
float getSingleMoveCost(const std::vector<int>& basePartitionIds, 
                       const std::vector<std::string>& tech_array, 
                       const std::vector<float>& aspect_ratios, 
                       const std::vector<float>& x_locations, 
                       const std::vector<float>& y_locations, 
                       const int blockId, 
                       const int fromPartitionId, 
                       const int toPartitionId, 
                       LibraryDicts* libraryDicts, 
                       const std::vector<block>& blocks, 
                       float cost_coefficient, 
                       float power_coefficient);

#endif // EVALUATOR_CPP_H 