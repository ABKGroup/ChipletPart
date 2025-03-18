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
  std::cout << "\n" << Console::BOLD << Console::BLUE << "Usage Instructions:" << Console::RESET << "\n\n";
  
  std::cout << Console::BOLD << "For partitioning: " << Console::RESET << programName
            << " <chiplet_io_file> <chiplet_layer_file> "
               "<chiplet_wafer_process_file> <chiplet_assembly_process_file> "
               "<chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> "
               "<reach> <separation> <tech> [--seed <random_seed>]\n\n";
               
  std::cout << Console::BOLD << "For evaluation: " << Console::RESET << programName
            << " <hypergraph_part> <chiplet_io_file> "
               "<chiplet_layer_file> "
               "<chiplet_wafer_process_file> <chiplet_assembly_process_file> "
               "<chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> "
               "<reach> <separation> <tech> [--seed <random_seed>]\n\n";
               
  std::cout << Console::BOLD << "For technology assignment: " << Console::RESET << programName
            << " <chiplet_io_file> "
               "<chiplet_layer_file> "
               "<chiplet_wafer_process_file> <chiplet_assembly_process_file> "
               "<chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> "
               "<reach> <separation> <tech1,tech2,...,techN> [--seed <random_seed>]\n\n";
               
  std::cout << Console::BOLD << "For genetic tech partitioning: " << Console::RESET << programName
            << " <chiplet_io_file> "
               "<chiplet_layer_file> "
               "<chiplet_wafer_process_file> <chiplet_assembly_process_file> "
               "<chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> "
               "<reach> <separation> --genetic-tech-part --tech-nodes <tech1 tech2 ...> "
               "[--generations <num>] [--population <num>] [--seed <random_seed>]\n\n";
               
  std::cout << Console::YELLOW << "  [--seed <random_seed>]" << Console::RESET << " is an optional parameter for reproducible results\n";
  std::cout << Console::YELLOW << "  [--generations <num>]" << Console::RESET << " is an optional parameter to set the number of generations (default: 50)\n";
  std::cout << Console::YELLOW << "  [--population <num>]" << Console::RESET << " is an optional parameter to set the population size (default: 50)\n";
}

// Function to parse a comma-separated list of technologies
std::vector<std::string> parseTechList(const std::string& techStr) {
  std::vector<std::string> techs;
  
  size_t start = 0;
  size_t pos = techStr.find(',');
  
  while (pos != std::string::npos) {
    std::string tech = techStr.substr(start, pos - start);
    if (!tech.empty()) {
      techs.push_back(tech);
    }
    start = pos + 1;
    pos = techStr.find(',', start);
  }
  
  // Add the last tech (or the only one if there were no commas)
  std::string lastTech = techStr.substr(start);
  if (!lastTech.empty()) {
    techs.push_back(lastTech);
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