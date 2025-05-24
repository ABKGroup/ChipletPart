///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "ChipletPart.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "evaluator_cpp.h" // Include the cost model evaluator_cpp.h instead of evaluator.h
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <numeric> // For std::accumulate

// Forward declarations for Console namespace
namespace Console {
  // ANSI color codes for terminal output
  const std::string RESET   = "\033[0m";
  const std::string BLACK   = "\033[30m";
  const std::string RED     = "\033[31m";
  const std::string GREEN   = "\033[32m";
  const std::string YELLOW  = "\033[33m";
  const std::string BLUE    = "\033[34m";
  const std::string MAGENTA = "\033[35m";
  const std::string CYAN    = "\033[36m";
  const std::string WHITE   = "\033[37m";
  
  // Text formatting
  const std::string BOLD    = "\033[1m";
  const std::string UNDERLINE = "\033[4m";
  
  void Info(const std::string& message);
  void Success(const std::string& message);
  void Warning(const std::string& message);
  void Error(const std::string& message);
  void Debug(const std::string& message);
  void Header(const std::string& message);
  void Subheader(const std::string& message);
}

// Function to print the application header
void printApplicationHeader() {
    std::cout << "\n\033[1;36m";  // Bold Cyan
    std::cout << "------------------------------------------------------------\n";
    std::cout << "            ChipletPart Partitioner / Evaluator             \n";
    std::cout << "                        Version: 1.0                        \n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Developed by: UC San Diego and UC Los Angeles               \n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "\033[0m\n";  // Reset color
}

// Function to display program header
void displayHeader() {
  const std::string separator(60, '-');
  const std::string title("ChipletPart Partitioner / Evaluator");
  const std::string version("Version: 1.0");
  const std::string developedBy("Developed by: UC San Diego and UC Los Angeles");

  std::cout << std::endl;
  std::cout << separator << std::endl;
  std::cout << std::setw((separator.size() + title.length()) / 2) << title << std::endl;
  std::cout << std::setw((separator.size() + version.length()) / 2) << version << std::endl;
  std::cout << separator << std::endl;
  std::cout << developedBy << std::endl;
  std::cout << separator << std::endl;
  std::cout << std::endl;
}

// Function to display usage instructions
void displayUsage(const char* programName) {
  std::cout << "Usage: " << programName << " [options] <arguments>" << std::endl;
  std::cout << "Standard mode: " << programName << " <io_file> <layer_file> <wafer_process_file> <assembly_process_file> <test_file> <netlist_file> <blocks_file> <reach> <separation> <tech_node> [--seed <value>]" << std::endl;
  std::cout << "Evaluation mode: " << programName << " <partition_file> <io_file> <layer_file> <wafer_process_file> <assembly_process_file> <test_file> <netlist_file> <blocks_file> <reach> <separation> <tech_node> [--seed <value>]" << std::endl;
  std::cout << "Canonical GA: " << programName << " <io_file> <layer_file> <wafer_process_file> <assembly_process_file> <test_file> <netlist_file> <blocks_file> <reach> <separation> --canonical-ga --tech-nodes <list> [--seed <value>] [--generations <value>] [--population <value>]" << std::endl;
  std::cout << "Tech Enumeration: " << programName << " <io_file> <layer_file> <wafer_process_file> <assembly_process_file> <test_file> <netlist_file> <blocks_file> <reach> <separation> --tech-enum --tech-nodes <list> [--max-partitions <value>] [--detailed-output] [--seed <value>]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --seed <value>        : Random seed for reproducible results (default: 42)" << std::endl;
  std::cout << "  --canonical-ga        : Use canonical genetic algorithm for technology assignment" << std::endl;
  std::cout << "  --tech-enum           : Enumerate all canonical technology assignments up to max partitions" << std::endl;
  std::cout << "  --tech-nodes <list>   : Comma-separated list of technology nodes (e.g., '7nm,14nm,28nm')" << std::endl;
  std::cout << "  --generations <value> : Number of generations for genetic algorithm (default: 50)" << std::endl;
  std::cout << "  --population <value>  : Population size for genetic algorithm (default: 50)" << std::endl;
  std::cout << "  --max-partitions <value> : Maximum number of partitions for tech enumeration (default: 4)" << std::endl;
  std::cout << "  --detailed-output     : Generate detailed output for tech enumeration" << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  " << programName << " io.xml layer.xml wafer.xml assembly.xml test.xml netlist.xml blocks.txt 0.5 0.25 7nm" << std::endl;
  std::cout << "  " << programName << " io.xml layer.xml wafer.xml assembly.xml test.xml netlist.xml blocks.txt 0.5 0.25 --canonical-ga --tech-nodes 7nm,14nm,28nm --seed 123" << std::endl;
  std::cout << "  " << programName << " io.xml layer.xml wafer.xml assembly.xml test.xml netlist.xml blocks.txt 0.5 0.25 --tech-enum --tech-nodes 7nm,14nm,28nm --max-partitions 3" << std::endl;
}

// Function to parse a comma-separated list of technologies
std::vector<std::string> parseTechList(const std::string& techStr) {
  std::vector<std::string> techs;
  
  // Handle empty string
  if (techStr.empty()) {
    return techs;
  }
  
  try {
    // Make a local copy of the input string to ensure memory safety
    std::string safe_tech_str(techStr.c_str());
    
    size_t start = 0;
    size_t pos = safe_tech_str.find(',');
    
    // Handle single tech node without commas
    if (pos == std::string::npos) {
      // Create a new string from the tech node
      std::string tech(safe_tech_str.c_str());
      techs.push_back(tech);
      return techs;
    }
    
    // Process comma-separated list
    while (pos != std::string::npos) {
      if (pos > start) { // Ensure we don't add empty strings from sequential commas
        // Get substring and ensure it's a proper new string
        std::string tech(safe_tech_str.substr(start, pos - start).c_str());
        if (!tech.empty()) {
          techs.push_back(tech);
        }
      }
      start = pos + 1;
      
      // Check bounds to prevent out-of-range errors
      if (start >= safe_tech_str.length()) {
        break;
      }
      
      pos = safe_tech_str.find(',', start);
    }
    
    // Add the last tech (or the only one if there were no commas)
    if (start < safe_tech_str.length()) {
      // Create a new string for the last tech node
      std::string lastTech(safe_tech_str.substr(start).c_str());
      if (!lastTech.empty()) {
        techs.push_back(lastTech);
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error parsing tech list: " << e.what() << std::endl;
    // Return an empty vector on error
    return std::vector<std::string>();
  }
  
  return techs;
}

// Function to safely convert string to float with error checking
float safeStof(const std::string& str, const std::string& paramName) {
  try {
    return std::stof(str);
  } catch (const std::exception& e) {
    throw std::runtime_error("Error parsing " + paramName + " value '" + str + "': " + e.what());
  }
}

// Function to safely convert string to int with error checking
int safeStoi(const std::string& str, const std::string& paramName) {
  try {
    return std::stoi(str);
  } catch (const std::exception& e) {
    throw std::runtime_error("Error parsing " + paramName + " value '" + str + "': " + e.what());
  }
}

// Function to check if a string argument is present and get its value
bool getArgValue(int argc, char* argv[], const std::string& option, std::string& value) {
  for (int i = 1; i < argc - 1; ++i) {
    if (option == argv[i]) {
      value = argv[i + 1];
      return true;
    }
  }
  return false;
}

// Function to check if a flag is present
bool hasFlag(int argc, char* argv[], const std::string& option) {
  for (int i = 1; i < argc; ++i) {
    if (option == argv[i]) {
      return true;
    }
  }
  return false;
}

// Add this function after other command handling functions
void run_tech_enum(
    int argc, 
    char** argv, 
    chiplet::ChipletPart& chiplet_part, 
    std::string& chiplet_io_file,
    std::string& chiplet_layer_file,
    std::string& chiplet_wafer_process_file,
    std::string& chiplet_assembly_process_file,
    std::string& chiplet_test_file,
    std::string& chiplet_netlist_file,
    std::string& chiplet_blocks_file,
    float reach,
    float separation,
    int seed) {
    
    // Parse tech nodes from command-line arguments or use defaults
    std::vector<std::string> tech_nodes;
    std::string tech_nodes_str;
    
    try {
        if (getArgValue(argc, argv, "--tech-nodes", tech_nodes_str)) {
            // Create a local copy of the string to ensure memory safety
            std::string safe_tech_str(tech_nodes_str.c_str());
            
            // Use safe string parsing to create the tech nodes vector
            tech_nodes = parseTechList(safe_tech_str);
            
            // Verify we have valid tech nodes
            if (!tech_nodes.empty()) {
                std::stringstream node_list;
                for (size_t i = 0; i < tech_nodes.size(); ++i) {
                    if (i > 0) node_list << ", ";
                    node_list << tech_nodes[i];
                }
                Console::Info("Using specified tech nodes: " + node_list.str());
            } else {
                Console::Warning("Failed to parse tech nodes, falling back to defaults");
                // Default technology nodes as individual strings
                tech_nodes.push_back(std::string("7nm"));
                tech_nodes.push_back(std::string("14nm"));
                tech_nodes.push_back(std::string("10nm"));
            }
        } else {
            // Default technology nodes as individual strings
            tech_nodes.push_back(std::string("7nm"));
            tech_nodes.push_back(std::string("14nm"));
            tech_nodes.push_back(std::string("10nm"));
            Console::Info("Using default tech nodes: 7nm, 14nm, 10nm");
        }
        
        // Get max_partitions parameter
        int max_partitions = 4; // Default max partitions
        std::string max_parts_str;
        if (getArgValue(argc, argv, "--max-partitions", max_parts_str)) {
            max_partitions = safeStoi(max_parts_str, "max_partitions");
            Console::Info("Maximum partitions set to: " + std::to_string(max_partitions));
        } else {
            Console::Info("Using default maximum partitions: 4");
        }
        
        // Check if detailed output is requested
        bool detailed_output = hasFlag(argc, argv, "--detailed-output");
        if (detailed_output) {
            Console::Info("Detailed output enabled");
        }
        
        // Run the technology enumeration algorithm
        if (tech_nodes.empty()) {
            Console::Error("No valid technology nodes found.");
            throw std::runtime_error("No valid technology nodes provided");
        }
        
        // Make sure files exist before running
        std::ifstream io_file(chiplet_io_file);
        if (!io_file.good()) {
            Console::Error("IO definitions file not found: " + chiplet_io_file);
            throw std::runtime_error("IO definitions file not found: " + chiplet_io_file);
        }
        
        // Call the technology enumeration method
        auto [best_cost, best_partition, best_tech_assignment] = chiplet_part.EnumerateTechAssignments(
            chiplet_io_file,
            chiplet_layer_file,
            chiplet_wafer_process_file,
            chiplet_assembly_process_file,
            chiplet_test_file,
            chiplet_netlist_file,
            chiplet_blocks_file,
            reach,
            separation,
            tech_nodes,
            max_partitions,
            detailed_output
        );
        
        // Display summary of results
        Console::Header("Technology Enumeration Summary");
        Console::Success("Best cost: " + std::to_string(best_cost));
        Console::Success("Number of partitions in best solution: " + std::to_string(best_tech_assignment.size()));
        
        // Format the best technology assignment as a string
        std::string best_tech_str = "[";
        for (size_t i = 0; i < best_tech_assignment.size(); i++) {
            best_tech_str += best_tech_assignment[i];
            if (i < best_tech_assignment.size() - 1) best_tech_str += ", ";
        }
        best_tech_str += "]";
        
        Console::Success("Best technology assignment: " + best_tech_str);
    } catch (const std::exception& e) {
        Console::Error("Exception in technology enumeration: " + std::string(e.what()));
        throw; // Re-throw to ensure the caller knows there was an error
    }
}

// Add this function after other command handling functions

void run_canonical_ga(
    int argc, 
    char** argv, 
    chiplet::ChipletPart& chiplet_part, 
    std::string& chiplet_io_file,
    std::string& chiplet_layer_file,
    std::string& chiplet_wafer_process_file,
    std::string& chiplet_assembly_process_file,
    std::string& chiplet_test_file,
    std::string& chiplet_netlist_file,
    std::string& chiplet_blocks_file,
    float reach,
    float separation,
    int seed,
    int population_size,
    int num_generations) {
    
    // Parse tech nodes from command-line arguments or use defaults
    std::vector<std::string> tech_nodes;
    std::string tech_nodes_str;
    
    try {
        if (getArgValue(argc, argv, "--tech-nodes", tech_nodes_str)) {
            // Create a local copy of the string to ensure memory safety
            std::string safe_tech_str(tech_nodes_str.c_str());
            
            // Use safe string parsing to create the tech nodes vector
            tech_nodes = parseTechList(safe_tech_str);
            
            // Verify we have valid tech nodes
            if (!tech_nodes.empty()) {
                std::stringstream node_list;
                for (size_t i = 0; i < tech_nodes.size(); ++i) {
                    if (i > 0) node_list << ", ";
                    node_list << tech_nodes[i];
                }
                Console::Info("Using specified tech nodes: " + node_list.str());
            } else {
                Console::Warning("Failed to parse tech nodes, falling back to defaults");
                // Default technology nodes as individual strings
                tech_nodes.push_back(std::string("7nm"));
                tech_nodes.push_back(std::string("14nm"));
                tech_nodes.push_back(std::string("10nm"));
            }
        } else {
            // Default technology nodes as individual strings
            tech_nodes.push_back(std::string("7nm"));
            tech_nodes.push_back(std::string("14nm"));
            tech_nodes.push_back(std::string("10nm"));
            Console::Info("Using default tech nodes: 7nm, 14nm, 10nm");
        }
        
        // Run the canonical genetic algorithm with check for empty tech_nodes
        if (tech_nodes.empty()) {
            Console::Error("No valid technology nodes found.");
            throw std::runtime_error("No valid technology nodes provided");
        }
        
        // Make sure files exist before running
        std::ifstream io_file(chiplet_io_file);
        if (!io_file.good()) {
            Console::Error("IO definitions file not found: " + chiplet_io_file);
            throw std::runtime_error("IO definitions file not found: " + chiplet_io_file);
        }
        
        // Call chiplet_part.CanonicalGeneticTechPart with the safe tech nodes vector
        chiplet_part.CanonicalGeneticTechPart(
            chiplet_io_file,
            chiplet_layer_file,
            chiplet_wafer_process_file,
            chiplet_assembly_process_file,
            chiplet_test_file,
            chiplet_netlist_file,
            chiplet_blocks_file,
            reach,
            separation,
            tech_nodes,
            population_size,
            num_generations,
            0.2,    // mutation_rate
            0.7,    // crossover_rate
            2,      // min_partitions
            8,      // max_partitions
            "canonical_ga_result"
        );
    } catch (const std::exception& e) {
        Console::Error("Exception in canonical GA: " + std::string(e.what()));
        throw; // Re-throw to ensure the caller knows there was an error
    }
}

int main(int argc, char *argv[]) {
  try {
    // Process seed parameter
    std::string seedStr;
    bool hasSeed = getArgValue(argc, argv, "--seed", seedStr);
    int seed = 42; // Default seed
    
    if (hasSeed) {
      seed = safeStoi(seedStr, "seed");
      Console::Info("Random seed set to " + std::to_string(seed));
    }
    
    // Process genetic tech partitioning parameters
    bool useGeneticTechPart = false;
    for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "--genetic-tech-part") {
        useGeneticTechPart = true;
        break;
      }
    }
    
    // Process canonical GA flag
    bool useCanonicalGA = false;
    for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "--canonical-ga") {
        useCanonicalGA = true;
        break;
      }
    }
    
    // Process technology enumeration flag
    bool useTechEnum = false;
    for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "--tech-enum") {
        useTechEnum = true;
        break;
      }
    }
    
    // Check for generations parameter
    std::string genStr;
    bool hasGenerations = getArgValue(argc, argv, "--generations", genStr);
    int generations = 50; // Default generations
    
    if (hasGenerations) {
      generations = safeStoi(genStr, "generations");
      Console::Info("Generations set to " + std::to_string(generations));
    }
    
    // Check for population parameter
    std::string popStr;
    bool hasPopulation = getArgValue(argc, argv, "--population", popStr);
    int population = 50; // Default population
    
    if (hasPopulation) {
      population = safeStoi(popStr, "population");
      Console::Info("Population size set to " + std::to_string(population));
    }
    
    // Print application header
    printApplicationHeader();
    
    // Create ChipletPart instance
    auto chiplet_part = std::make_shared<chiplet::ChipletPart>(seed);
    
    // Technology enumeration mode
    if (useTechEnum) {
      try {
        // Create a clean list of arguments without optional flags and their values
        std::vector<std::string> cleanArgs;
        for (int i = 1; i < argc; i++) {
          std::string arg = argv[i];
          if (arg == "--tech-enum" || arg == "--detailed-output") {
            continue; // Skip the flag itself
          }
          if (arg == "--seed" || arg == "--max-partitions" || arg == "--tech-nodes") {
            i++; // Skip the flag and its value
            continue;
          }
          cleanArgs.push_back(arg);
        }
        
        // Check if we have enough arguments for technology enumeration
        if (cleanArgs.size() < 9) {
          Console::Error("Not enough required arguments for technology enumeration");
          displayUsage(argv[0]);
          return 1;
        }
        
        // Now process the required positional arguments
        std::string io_definitions_file = cleanArgs[0];
        std::string layer_definitions_file = cleanArgs[1];
        std::string wafer_process_definitions_file = cleanArgs[2];
        std::string assembly_process_definitions_file = cleanArgs[3];
        std::string test_definitions_file = cleanArgs[4];
        std::string block_level_netlist_file = cleanArgs[5];
        std::string block_definitions_file = cleanArgs[6];
        float reach = safeStof(cleanArgs[7], "reach");
        float separation = safeStof(cleanArgs[8], "separation");
        
        // Run the technology enumeration algorithm
        run_tech_enum(
            argc,
            argv,
            *chiplet_part,
            io_definitions_file,
            layer_definitions_file,
            wafer_process_definitions_file,
            assembly_process_definitions_file,
            test_definitions_file,
            block_level_netlist_file,
            block_definitions_file,
            reach,
            separation,
            seed
        );
        
        return 0;
      } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
      } catch (...) {
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return 1;
      }
    }
    
    // For genetic tech partitioning, we need a different approach to argument parsing
    if (useGeneticTechPart) {
      // Create a list of all arguments to parse
      std::vector<std::string> args;
      for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
      }
      
      // Find the indices of required fixed arguments
      int firstOptionalIdx = -1;
      for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--genetic-tech-part" || args[i] == "--seed" || 
            args[i] == "--generations" || args[i] == "--population" ||
            args[i] == "--tech-nodes") {
          if (firstOptionalIdx == -1 || static_cast<int>(i) < firstOptionalIdx) {
            firstOptionalIdx = i;
          }
        }
      }
      
      if (firstOptionalIdx < 10) {
        Console::Error("Not enough required arguments for genetic tech partitioning");
        displayUsage(argv[0]);
        return 1;
      }
      
      // Parse the standard arguments (first 10 args before any optional ones)
      std::string io_definitions_file = args[0];
      std::string layer_definitions_file = args[1];
      std::string wafer_process_definitions_file = args[2];
      std::string assembly_process_definitions_file = args[3];
      std::string test_definitions_file = args[4];
      std::string block_level_netlist_file = args[5];
      std::string block_definitions_file = args[6];
      float reach = safeStof(args[7], "reach");
      float separation = safeStof(args[8], "separation");
      
      // Parse tech nodes from --tech-nodes option
      std::vector<std::string> techNodes;
      std::string techNodesStr;
      if (getArgValue(argc, argv, "--tech-nodes", techNodesStr)) {
        // Find all tech nodes after --tech-nodes
        int techNodesIdx = -1;
        for (size_t i = 0; i < args.size(); i++) {
          if (args[i] == "--tech-nodes") {
            techNodesIdx = i;
            break;
          }
        }
        
        if (techNodesIdx >= 0) {
          // Add all arguments after --tech-nodes until the next option
          for (size_t i = techNodesIdx + 1; i < args.size(); i++) {
            if (args[i][0] == '-' && args[i][1] == '-') {
              break;
            }
            techNodes.push_back(args[i]);
          }
        }
      }
      
      if (techNodes.empty()) {
        Console::Error("No tech nodes specified for genetic tech partitioning");
        displayUsage(argv[0]);
        return 1;
      }
      
      Console::Info("Running genetic tech partitioning");
      Console::Info("Tech nodes: " + std::accumulate(techNodes.begin(), techNodes.end(), std::string(),
                                             [](const std::string& a, const std::string& b) {
                                               return a + (a.empty() ? "" : ", ") + b;
                                             }));
      
      // Set seed if provided
      if (hasSeed) {
        chiplet_part->SetSeed(seed);
      }
      
      // Run the genetic tech partitioning
      chiplet_part->GeneticTechPart(
          io_definitions_file, layer_definitions_file, wafer_process_definitions_file,
          assembly_process_definitions_file, test_definitions_file, block_level_netlist_file, 
          block_definitions_file, reach, separation, techNodes,
          population, generations);
      
      return 0;
    }
    // Special handling for canonical GA mode
    else if (useCanonicalGA) {
      try {
        // Create a clean list of arguments without optional flags and their values
        std::vector<std::string> cleanArgs;
        for (int i = 1; i < argc; i++) {
          std::string arg = argv[i];
          if (arg == "--canonical-ga") {
            continue; // Skip the flag itself
          }
          if (arg == "--seed" || arg == "--generations" || arg == "--population" || arg == "--tech-nodes") {
            i++; // Skip the flag and its value
            continue;
          }
          cleanArgs.push_back(arg);
        }
        
        // Check if we have enough arguments for canonical GA
        if (cleanArgs.size() < 9) {
          Console::Error("Not enough required arguments for canonical GA");
          displayUsage(argv[0]);
          return 1;
        }
        
        // Now process the required positional arguments
        std::string io_definitions_file = cleanArgs[0];
        std::string layer_definitions_file = cleanArgs[1];
        std::string wafer_process_definitions_file = cleanArgs[2];
        std::string assembly_process_definitions_file = cleanArgs[3];
        std::string test_definitions_file = cleanArgs[4];
        std::string block_level_netlist_file = cleanArgs[5];
        std::string block_definitions_file = cleanArgs[6];
        float reach = safeStof(cleanArgs[7], "reach");
        float separation = safeStof(cleanArgs[8], "separation");
        
        // Get tech nodes
        std::vector<std::string> techNodes;
        std::string techNodesStr;
        if (getArgValue(argc, argv, "--tech-nodes", techNodesStr)) {
          techNodes = parseTechList(techNodesStr);
          Console::Info("Using specified tech nodes: " + techNodesStr);
        } else {
          // Default technology nodes
          techNodes = {"7nm", "14nm", "28nm"};
          Console::Info("Using default tech nodes: 7nm, 14nm, 28nm");
        }
        
        // Run the canonical GA algorithm
        run_canonical_ga(
            argc,
            argv,
            *chiplet_part,
            io_definitions_file,
            layer_definitions_file,
            wafer_process_definitions_file,
            assembly_process_definitions_file,
            test_definitions_file,
            block_level_netlist_file,
            block_definitions_file,
            reach,
            separation,
            seed,
            population,
            generations
        );
        
        return 0;
      } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
      } catch (...) {
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return 1;
      }
    }
    
    // The rest of the code for standard partitioning modes
    int effectiveArgc = argc;
    
    // Adjust effectiveArgc if --seed is used
    if (hasSeed) {
      effectiveArgc -= 2; // Remove --seed and its value from the count
    }
    
    // Check if we have the correct number of arguments
    if (effectiveArgc < 11 || effectiveArgc > 12) {
      displayUsage(argv[0]);
      return 1;
    }
    
    // Set seed if provided
    if (hasSeed) {
      chiplet_part->SetSeed(seed);
    }
    
    // Determine the actual indices for the arguments
    int argOffset = 0;
    for (int i = 1; i < argc; i++) {
      if (std::string(argv[i]) == "--seed") {
        i++; // Skip the seed value
        argOffset += 2;
      }
    }
    
    if (effectiveArgc == 11) {
      // Partitioning mode with XML input
      std::string io_definitions_file = argv[1 + argOffset * (argv[1] == std::string("--seed"))];
      std::string layer_definitions_file = argv[2 + argOffset * (argv[2] == std::string("--seed") || argv[1] == std::string("--seed"))];
      std::string wafer_process_definitions_file = argv[3 + argOffset * (argv[3] == std::string("--seed") || argv[2] == std::string("--seed") || argv[1] == std::string("--seed"))];
      std::string assembly_process_definitions_file = argv[4 + argOffset * (argv[4] == std::string("--seed") || argv[3] == std::string("--seed") || argv[2] == std::string("--seed") || argv[1] == std::string("--seed"))];
      std::string test_definitions_file = argv[5 + argOffset * (argv[5] == std::string("--seed") || argv[4] == std::string("--seed") || argv[3] == std::string("--seed") || argv[2] == std::string("--seed") || argv[1] == std::string("--seed"))];
      std::string block_level_netlist_file = argv[6 + argOffset * (argv[6] == std::string("--seed") || argv[5] == std::string("--seed") || argv[4] == std::string("--seed") || argv[3] == std::string("--seed") || argv[2] == std::string("--seed") || argv[1] == std::string("--seed"))];
      std::string block_definitions_file = argv[7 + argOffset * (argv[7] == std::string("--seed") || argv[6] == std::string("--seed") || argv[5] == std::string("--seed") || argv[4] == std::string("--seed") || argv[3] == std::string("--seed") || argv[2] == std::string("--seed") || argv[1] == std::string("--seed"))];
      
      // The logic above is complex and error-prone, let's simplify it:
      // Since the seed parameter can appear anywhere, we need a cleaner approach
      
      // Create a new array without the seed parameters
      std::vector<std::string> cleanArgs;
      for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--seed") {
          i++; // Skip the seed value
          continue;
        }
        if (i > 0) { // Skip program name
          cleanArgs.push_back(arg);
        }
      }
      
      // Now use the cleaned args
      io_definitions_file = cleanArgs[0];
      layer_definitions_file = cleanArgs[1];
      wafer_process_definitions_file = cleanArgs[2];
      assembly_process_definitions_file = cleanArgs[3];
      test_definitions_file = cleanArgs[4];
      block_level_netlist_file = cleanArgs[5];
      block_definitions_file = cleanArgs[6];
      
      float reach = safeStof(cleanArgs[7], "reach");
      float separation = safeStof(cleanArgs[8], "separation");
      std::string tech = cleanArgs[9];
      
      if (tech.find(',') != std::string::npos) {
        // Technology assignment mode (multiple technologies)
        std::vector<std::string> techs = parseTechList(tech);
        
        chiplet_part->TechAssignPartition(
            io_definitions_file, layer_definitions_file, wafer_process_definitions_file,
            assembly_process_definitions_file, test_definitions_file, block_level_netlist_file, 
            block_definitions_file, reach, separation, techs);
      } else {
        // Single technology partitioning
        std::cout << "[INFO] Partitioning using XML input files" << std::endl;
        
        chiplet_part->Partition(
            io_definitions_file, layer_definitions_file, wafer_process_definitions_file,
            assembly_process_definitions_file, test_definitions_file, block_level_netlist_file, 
            block_definitions_file, reach, separation, tech);
      }
    } else if (effectiveArgc == 12) {
      // Evaluation mode
      // Use the same cleanArgs approach for evaluation mode
      std::vector<std::string> cleanArgs;
      for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--seed") {
          i++; // Skip the seed value
          continue;
        }
        if (i > 0) { // Skip program name
          cleanArgs.push_back(arg);
        }
      }
      
      std::string hypergraph_part = cleanArgs[0];
      std::string io_definitions_file = cleanArgs[1];
      std::string layer_definitions_file = cleanArgs[2];
      std::string wafer_process_definitions_file = cleanArgs[3];
      std::string assembly_process_definitions_file = cleanArgs[4];
      std::string test_definitions_file = cleanArgs[5];
      std::string block_level_netlist_file = cleanArgs[6];
      std::string block_definitions_file = cleanArgs[7];
      
      float reach = safeStof(cleanArgs[8], "reach");
      float separation = safeStof(cleanArgs[9], "separation");
      std::string tech = cleanArgs[10];
      
      std::cout << "[INFO] Evaluating partition" << std::endl;
      
      // Read the XML files to create the hypergraph
      chiplet_part->ReadChipletGraphFromXML(io_definitions_file, block_level_netlist_file, block_definitions_file);
      
      chiplet_part->EvaluatePartition(
          hypergraph_part, io_definitions_file, layer_definitions_file, wafer_process_definitions_file,
          assembly_process_definitions_file, test_definitions_file, block_level_netlist_file, 
          block_definitions_file, reach, separation, tech);
    }
    
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
