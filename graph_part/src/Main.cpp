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
#include <iomanip>

int main(int argc, char *argv[]) {

  const std::string separator(60, '-'); // Creates a separator line
  const std::string title("ChipletPart Partitioner / Evaluator");
  const std::string version("Version: 1.0");
  const std::string developedBy(
      "Developed by: UC San Diego and UC Los Angeles");

  // Print formatted startup message
  std::cout << std::endl;
  std::cout << separator << std::endl;
  std::cout << std::setw((separator.size() + title.length()) / 2) << title
            << std::endl;
  std::cout << std::setw((separator.size() + version.length()) / 2) << version
            << std::endl;
  std::cout << separator << std::endl;
  std::cout << developedBy << std::endl;
  std::cout << separator << std::endl;
  std::cout << std::endl;

  if (argc == 12) {
    chiplet::ChipletPartPtr chiplet_part =
        std::make_shared<chiplet::ChipletPart>();
    std::cout << "[INFO] Reading the chiplet graph " << argv[1] << std::endl;
    chiplet_part->ReadChipletGraph(argv[1], argv[2]);
    float reach = std::stof(argv[9]);
    float separation = std::stof(argv[10]);

    // check if argv[11] is an array of technologies
    // if it is an array then call TechAssignPart()
    // if it is not an array then call Partition()
    std::string tech = argv[11];
    if (tech.find(",") != std::string::npos) {
      // convert argv[11] to a vector of strings
      std::vector<std::string> techs;
      std::string delimiter = ",";
      size_t pos = 0;
      std::string token;
      while ((pos = tech.find(delimiter)) != std::string::npos) {
        token = tech.substr(0, pos);
        techs.push_back(token);
        tech.erase(0, pos + delimiter.length());
      }
      techs.push_back(tech);
      chiplet_part->TechAssignPartition(argv[1], argv[2], argv[3], argv[4],
                                        argv[5], argv[6], argv[7], argv[8],
                                        reach, separation, techs);
    } else {
      std::cout << "[INFO] Partitioning the chiplet graph " << argv[1]
                << std::endl;
      chiplet_part->Partition(argv[1], argv[2], argv[3], argv[4], argv[5],
                              argv[6], argv[7], argv[8], reach, separation,
                              argv[11]);
    }
    return 1;
  } else if (argc == 13) {
    chiplet::ChipletPartPtr chiplet_part =
        std::make_shared<chiplet::ChipletPart>();
    std::cout << "[INFO] Reading the chiplet graph " << argv[1] << std::endl;
    chiplet_part->ReadChipletGraph(argv[1], argv[3]);
    float reach = std::stof(argv[10]);
    float separation = std::stof(argv[11]);
    std::cout << "[INFO] Evaluating the partitioning of the chiplet graph "
              << argv[1] << std::endl;
    chiplet_part->EvaluatePartition(argv[1], argv[2], argv[3], argv[4], argv[5],
                                    argv[6], argv[7], argv[8], argv[9], reach,
                                    separation, argv[12]);
    return 1;
  } else {
    std::cerr
        << "Usage for partitioning: " << argv[0]
        << " <hypergraph_file> <chiplet_io_file> <chiplet_layer_file> "
           "<chiplet_wafer_process_file> <chiplet_assembly_process_file> "
           "<chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> "
           "<reach> <separation> <tech>\n"
        << "Usage for evaluation: " << argv[0]
        << " <hypergraph_file> <hypergraph_part> <chiplet_io_file> "
           "<chiplet_layer_file> "
           "<chiplet_wafer_process_file> <chiplet_assembly_process_file> "
           "<chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> "
           "<reach> <separation> <tech>\n"
        << "Usage for technology assignment: " << argv[0]
        << " <hypergraph_file> <hypergraph_part> <chiplet_io_file> "
           "<chiplet_layer_file> "
           "<chiplet_wafer_process_file> <chiplet_assembly_process_file> "
           "<chiplet_test_file> <chiplet_netlist_file> <chiplet_blocks_file> "
           "<reach> <separation> <tech array>\n"
        << std::endl;
    return 1;
  }
}