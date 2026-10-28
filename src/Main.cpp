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
#include "Console.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Command-line contract
// ---------------------------------------------------------------------------
//
// Every mode takes the same 9 design-description positionals:
//
//   <io> <layer> <wafer_process> <assembly_process> <test> <netlist> <blocks>
//   <reach> <separation>
//
// followed by either a 10th positional <tech> (standard partitioning) or a
// mode flag (--canonical-ga, --tech-enum, --genetic-tech-part). Evaluation
// mode prepends a <partition_file> positional. Options may appear anywhere.
//
// Technology nodes the cost model knows about (see
// cost_model/src/ConstructChip.cpp): 45nm, 16nm, 14nm, 10nm, 7nm.

const std::vector<std::string> kDefaultTechNodes = {"7nm", "10nm", "14nm"};
constexpr int kNumDesignPositionals = 9;

enum class Mode { Standard, Evaluate, CanonicalGA, TechEnum, GeneticTechPart };

struct Options {
  Mode mode = Mode::Standard;
  std::vector<std::string> positionals;

  int seed = 42;
  bool has_seed = false;

  int generations = 50;
  int population = 50;
  int max_partitions = 4;         // --tech-enum default
  bool has_max_partitions = false;
  bool detailed_output = false;

  std::vector<std::string> tech_nodes;  // --tech-nodes
  std::string output_dir;               // --output-dir (empty = cwd)

  bool floorplan_aware = true;          // --[no-]floorplan-aware
  int fp_check_steps = 40;              // --fp-check-budget S,P
  int fp_check_perturbs = 25;
  int fp_steps = 1000;                  // --fp-steps
  int fp_perturbs = 1000;               // --fp-perturbs
  int fp_workers = 0;                   // --fp-workers (0 = auto)
  bool help = false;
};

void displayUsage(const char* programName) {
  const std::string design =
      "<io_file> <layer_file> <wafer_process_file> <assembly_process_file> "
      "<test_file> <netlist_file> <blocks_file> <reach> <separation>";
  std::cout
      << "Usage: " << programName << " [options] <arguments>\n"
      << "\n"
      << "Standard mode:   " << programName << " " << design << " <tech_node>\n"
      << "Tech-assign mode:" << programName << " " << design << " <tech_a>,<tech_b>[,...]\n"
      << "                 (same algorithm as --genetic-tech-part)\n"
      << "Evaluation mode: " << programName << " <partition_file> " << design << " <tech_node>\n"
      << "Canonical GA:    " << programName << " " << design
      << " --canonical-ga [--tech-nodes <list>] [--generations N] [--population N] [--max-partitions N]\n"
      << "Tech enumeration:" << programName << " " << design
      << " --tech-enum [--tech-nodes <list>] [--max-partitions N] [--detailed-output]\n"
      << "Genetic tech:    " << programName << " " << design
      << " --genetic-tech-part --tech-nodes <list> [--generations N] [--population N] [--max-partitions N]\n"
      << "\n"
      << "Options (may appear anywhere on the command line):\n"
      << "  --seed <int>             Random seed (default: 42)\n"
      << "  --tech-nodes <list>      Comma-separated technology nodes, e.g. 7nm,10nm,14nm\n"
      << "                           (default: 7nm,10nm,14nm). Known nodes: 45nm 16nm 14nm 10nm 7nm\n"
      << "  --generations <int>      GA generations (default: 50)\n"
      << "  --population <int>       GA population size (default: 50)\n"
      << "  --max-partitions <int>   Max chiplets to consider (default: 4 for --tech-enum, 8 for the GAs)\n"
      << "  --detailed-output        Extra per-assignment output for --tech-enum\n"
      << "  --output-dir <dir>       Directory for all result files (default: current directory)\n"
      << "  --floorplan-aware        Gate every FM move / KL swap on a short floorplan probe so the\n"
      << "                           reported partition is I/O-reach feasible (default: on)\n"
      << "  --no-floorplan-aware     Pure cost-driven refinement (faster, may be infeasible)\n"
      << "  --fp-check-budget <S,P>  Per-move probe: SA steps,perturbations (default: 40,25)\n"
      << "  --fp-steps <int>         Final floorplan: SA steps per walker (default: 1000)\n"
      << "  --fp-perturbs <int>      Final floorplan: perturbations per step (default: 1000)\n"
      << "  --fp-workers <int>       Final floorplan: parallel SA walkers (default: min(threads,4))\n"
      << "  -h, --help               Show this message\n"
      << "\n"
      << "Positional semantics:\n"
      << "  <reach>       Default IO reach (mm) for net types that have no 'reach' attribute in the IO file\n"
      << "  <separation>  Minimum spacing between chiplets used by the floorplanner (mm)\n"
      << "\n"
      << "Examples:\n"
      << "  " << programName << " io.xml layer.xml wafer.xml assembly.xml test.xml netlist.xml blocks.txt 0.5 0.25 7nm\n"
      << "  " << programName << " io.xml layer.xml wafer.xml assembly.xml test.xml netlist.xml blocks.txt 0.5 0.25 "
         "--canonical-ga --tech-nodes 7nm,14nm --seed 123 --output-dir results/\n"
      << "  " << programName << " io.xml layer.xml wafer.xml assembly.xml test.xml netlist.xml blocks.txt 0.5 0.25 "
         "--tech-enum --tech-nodes 7nm,10nm,14nm --max-partitions 3\n";
}

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

std::vector<std::string> parseTechList(const std::string& techStr) {
  std::vector<std::string> techs;
  std::string current;
  for (char c : techStr) {
    if (c == ',' || c == ' ') {
      if (!current.empty()) techs.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  if (!current.empty()) techs.push_back(current);
  return techs;
}

bool looksLikeTechNode(const std::string& s) {
  static const std::regex re("^[0-9]+(\\.[0-9]+)?nm$");
  return std::regex_match(s, re);
}

float parseFloat(const std::string& str, const std::string& paramName) {
  try {
    size_t idx = 0;
    float v = std::stof(str, &idx);
    if (idx != str.size()) throw std::invalid_argument("trailing characters");
    return v;
  } catch (const std::exception& e) {
    throw std::runtime_error("Invalid " + paramName + " value '" + str + "': " + e.what());
  }
}

int parseInt(const std::string& str, const std::string& paramName) {
  try {
    size_t idx = 0;
    int v = std::stoi(str, &idx);
    if (idx != str.size()) throw std::invalid_argument("trailing characters");
    return v;
  } catch (const std::exception& e) {
    throw std::runtime_error("Invalid " + paramName + " value '" + str + "': " + e.what());
  }
}

std::string join(const std::vector<std::string>& v, const std::string& sep = ", ") {
  std::string out;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) out += sep;
    out += v[i];
  }
  return out;
}

Options parseArgs(int argc, char* argv[]) {
  Options opt;
  auto needValue = [&](int& i, const std::string& flag) -> std::string {
    if (i + 1 >= argc) throw std::runtime_error("Option " + flag + " requires a value");
    return argv[++i];
  };

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      opt.help = true;
    } else if (arg == "--canonical-ga") {
      opt.mode = Mode::CanonicalGA;
    } else if (arg == "--tech-enum") {
      opt.mode = Mode::TechEnum;
    } else if (arg == "--genetic-tech-part") {
      opt.mode = Mode::GeneticTechPart;
    } else if (arg == "--detailed-output") {
      opt.detailed_output = true;
    } else if (arg == "--seed") {
      opt.seed = parseInt(needValue(i, arg), "seed");
      opt.has_seed = true;
    } else if (arg == "--generations") {
      opt.generations = parseInt(needValue(i, arg), "generations");
    } else if (arg == "--population") {
      opt.population = parseInt(needValue(i, arg), "population");
    } else if (arg == "--max-partitions") {
      opt.max_partitions = parseInt(needValue(i, arg), "max-partitions");
      opt.has_max_partitions = true;
    } else if (arg == "--output-dir") {
      opt.output_dir = needValue(i, arg);
    } else if (arg == "--floorplan-aware") {
      opt.floorplan_aware = true;
    } else if (arg == "--no-floorplan-aware") {
      opt.floorplan_aware = false;
    } else if (arg == "--fp-check-budget") {
      const std::string v = needValue(i, arg);
      const auto comma = v.find(',');
      if (comma == std::string::npos) {
        throw std::runtime_error("--fp-check-budget expects <steps>,<perturbations>");
      }
      opt.fp_check_steps = parseInt(v.substr(0, comma), "fp-check-budget steps");
      opt.fp_check_perturbs = parseInt(v.substr(comma + 1), "fp-check-budget perturbations");
    } else if (arg == "--fp-steps") {
      opt.fp_steps = parseInt(needValue(i, arg), "fp-steps");
    } else if (arg == "--fp-perturbs") {
      opt.fp_perturbs = parseInt(needValue(i, arg), "fp-perturbs");
    } else if (arg == "--fp-workers") {
      opt.fp_workers = parseInt(needValue(i, arg), "fp-workers");
    } else if (arg == "--tech-nodes") {
      opt.tech_nodes = parseTechList(needValue(i, arg));
      // Legacy form: `--tech-nodes 7nm 14nm 28nm`. Absorb following tokens
      // that are unambiguously tech nodes so old scripts keep working.
      while (i + 1 < argc && looksLikeTechNode(argv[i + 1])) {
        opt.tech_nodes.push_back(argv[++i]);
      }
      if (opt.tech_nodes.empty()) {
        throw std::runtime_error("--tech-nodes was given an empty list");
      }
    } else if (arg.rfind("--", 0) == 0) {
      throw std::runtime_error("Unknown option: " + arg);
    } else {
      opt.positionals.push_back(arg);
    }
  }

  if (opt.generations <= 0) throw std::runtime_error("--generations must be positive");
  if (opt.population <= 0) throw std::runtime_error("--population must be positive");
  if (opt.has_max_partitions && opt.max_partitions <= 0) {
    throw std::runtime_error("--max-partitions must be positive");
  }
  if (opt.fp_check_steps <= 0 || opt.fp_check_perturbs <= 0) {
    throw std::runtime_error("--fp-check-budget values must be positive");
  }
  if (opt.fp_steps <= 0) throw std::runtime_error("--fp-steps must be positive");
  if (opt.fp_perturbs <= 0) throw std::runtime_error("--fp-perturbs must be positive");
  if (opt.fp_workers < 0) throw std::runtime_error("--fp-workers must be non-negative");

  // Evaluation mode is the standard mode with one extra leading positional.
  if (opt.mode == Mode::Standard && opt.positionals.size() == kNumDesignPositionals + 2) {
    opt.mode = Mode::Evaluate;
  }
  return opt;
}

struct DesignFiles {
  std::string io, layer, wafer, assembly, test, netlist, blocks;
  float reach = 0.0f;
  float separation = 0.0f;
};

DesignFiles readDesign(const std::vector<std::string>& pos, size_t offset) {
  DesignFiles d;
  d.io = pos[offset + 0];
  d.layer = pos[offset + 1];
  d.wafer = pos[offset + 2];
  d.assembly = pos[offset + 3];
  d.test = pos[offset + 4];
  d.netlist = pos[offset + 5];
  d.blocks = pos[offset + 6];
  d.reach = parseFloat(pos[offset + 7], "reach");
  d.separation = parseFloat(pos[offset + 8], "separation");

  for (const std::string* f : {&d.io, &d.layer, &d.wafer, &d.assembly, &d.test, &d.netlist, &d.blocks}) {
    if (!std::filesystem::exists(*f)) {
      throw std::runtime_error("Input file not found: " + *f);
    }
  }
  if (d.reach <= 0.0f) throw std::runtime_error("<reach> must be positive");
  if (d.separation < 0.0f) throw std::runtime_error("<separation> must be non-negative");
  return d;
}

void requirePositionals(const Options& opt, size_t expected, const char* modeName, const char* prog) {
  if (opt.positionals.size() != expected) {
    Console::Error(std::string(modeName) + " expects " + std::to_string(expected) +
                   " positional arguments, got " + std::to_string(opt.positionals.size()));
    displayUsage(prog);
    throw std::runtime_error("wrong number of positional arguments");
  }
}

std::vector<std::string> techNodesOrDefault(const Options& opt) {
  if (!opt.tech_nodes.empty()) {
    Console::Info("Using specified tech nodes: " + join(opt.tech_nodes));
    return opt.tech_nodes;
  }
  Console::Info("Using default tech nodes: " + join(kDefaultTechNodes));
  return kDefaultTechNodes;
}

}  // namespace

int main(int argc, char* argv[]) {
  Options opt;
  try {
    opt = parseArgs(argc, argv);
  } catch (const std::exception& e) {
    Console::Error(e.what());
    displayUsage(argv[0]);
    return 1;
  }
  if (opt.help || argc == 1) {
    displayUsage(argv[0]);
    return opt.help ? 0 : 1;
  }

  try {
    printApplicationHeader();

    auto chiplet_part = std::make_shared<chiplet::ChipletPart>(opt.seed);
    if (opt.has_seed) {
      Console::Info("Random seed set to " + std::to_string(opt.seed));
    }
    chiplet_part->SetOutputDir(opt.output_dir);
    chiplet_part->SetFloorplanAware(opt.floorplan_aware);
    chiplet_part->SetFloorplanCheckBudget(opt.fp_check_steps, opt.fp_check_perturbs);
    chiplet_part->SetFinalFloorplanBudget(opt.fp_steps, opt.fp_perturbs);
    chiplet_part->SetFloorplanWorkers(opt.fp_workers);
    Console::Info("Output directory: " + chiplet_part->GetOutputDir());

    switch (opt.mode) {
      case Mode::Standard: {
        requirePositionals(opt, kNumDesignPositionals + 1, "Standard mode", argv[0]);
        DesignFiles d = readDesign(opt.positionals, 0);
        const std::string tech = opt.positionals[kNumDesignPositionals];
        chiplet_part->SetDefaultReach(d.reach);
        chiplet_part->SetSeparation(d.separation);

        if (tech.find(',') != std::string::npos) {
          // Technology-assignment mode (multiple technologies) — this is the
          // GeneticTechPartitioner, same as --genetic-tech-part.
          std::vector<std::string> techs = parseTechList(tech);
          Console::Info("Tech-assignment partitioning with nodes: " + join(techs));
          chiplet_part->GeneticTechPart(d.io, d.layer, d.wafer, d.assembly, d.test, d.netlist,
                                        d.blocks, d.reach, d.separation, techs,
                                        opt.population, opt.generations,
                                        0.2f, 0.7f, 2,
                                        opt.has_max_partitions ? opt.max_partitions : 8);
        } else {
          Console::Info("Partitioning using XML input files");
          chiplet_part->Partition(d.io, d.layer, d.wafer, d.assembly, d.test, d.netlist,
                                  d.blocks, d.reach, d.separation, tech);
        }
        break;
      }

      case Mode::Evaluate: {
        requirePositionals(opt, kNumDesignPositionals + 2, "Evaluation mode", argv[0]);
        const std::string partition_file = opt.positionals[0];
        if (!std::filesystem::exists(partition_file)) {
          throw std::runtime_error("Partition file not found: " + partition_file);
        }
        DesignFiles d = readDesign(opt.positionals, 1);
        const std::string tech = opt.positionals[kNumDesignPositionals + 1];
        chiplet_part->SetDefaultReach(d.reach);
        chiplet_part->SetSeparation(d.separation);

        Console::Info("Evaluating partition " + partition_file);
        chiplet_part->ReadChipletGraphFromXML(d.io, d.netlist, d.blocks);
        chiplet_part->EvaluatePartition(partition_file, d.io, d.layer, d.wafer, d.assembly,
                                        d.test, d.netlist, d.blocks, d.reach, d.separation, tech);
        break;
      }

      case Mode::CanonicalGA: {
        requirePositionals(opt, kNumDesignPositionals, "Canonical GA", argv[0]);
        DesignFiles d = readDesign(opt.positionals, 0);
        chiplet_part->SetDefaultReach(d.reach);
        chiplet_part->SetSeparation(d.separation);
        std::vector<std::string> techs = techNodesOrDefault(opt);
        const int max_parts = opt.has_max_partitions ? opt.max_partitions : 8;
        Console::Info("Canonical GA: population " + std::to_string(opt.population) +
                      ", generations " + std::to_string(opt.generations) +
                      ", max partitions " + std::to_string(max_parts));
        // Mutation/crossover/min-partitions are the values the GA previously
        // forced internally; they are now plain defaults passed from here.
        chiplet_part->CanonicalGeneticTechPart(d.io, d.layer, d.wafer, d.assembly, d.test,
                                               d.netlist, d.blocks, d.reach, d.separation, techs,
                                               opt.population, opt.generations,
                                               0.08f,  // mutation_rate
                                               0.9f,   // crossover_rate
                                               1,      // min_partitions
                                               max_parts,
                                               "canonical_ga");
        break;
      }

      case Mode::TechEnum: {
        requirePositionals(opt, kNumDesignPositionals, "Tech enumeration", argv[0]);
        DesignFiles d = readDesign(opt.positionals, 0);
        chiplet_part->SetDefaultReach(d.reach);
        chiplet_part->SetSeparation(d.separation);
        std::vector<std::string> techs = techNodesOrDefault(opt);
        Console::Info("Maximum partitions: " + std::to_string(opt.max_partitions));
        if (opt.detailed_output) Console::Info("Detailed output enabled");

        auto [best_cost, best_partition, best_tech] = chiplet_part->EnumerateTechAssignments(
            d.io, d.layer, d.wafer, d.assembly, d.test, d.netlist, d.blocks, d.reach,
            d.separation, techs, opt.max_partitions, opt.detailed_output);

        Console::Header("Technology Enumeration Summary");
        Console::Success("Best cost: " + std::to_string(best_cost));
        Console::Success("Number of partitions in best solution: " + std::to_string(best_tech.size()));
        Console::Success("Best technology assignment: [" + join(best_tech) + "]");
        break;
      }

      case Mode::GeneticTechPart: {
        requirePositionals(opt, kNumDesignPositionals, "Genetic tech partitioning", argv[0]);
        DesignFiles d = readDesign(opt.positionals, 0);
        chiplet_part->SetDefaultReach(d.reach);
        chiplet_part->SetSeparation(d.separation);
        std::vector<std::string> techs = techNodesOrDefault(opt);
        Console::Info("Running genetic tech partitioning with nodes: " + join(techs));
        chiplet_part->GeneticTechPart(d.io, d.layer, d.wafer, d.assembly, d.test, d.netlist,
                                      d.blocks, d.reach, d.separation, techs,
                                      opt.population, opt.generations,
                                      0.2f, 0.7f, 2,
                                      opt.has_max_partitions ? opt.max_partitions : 8);
        break;
      }
    }
    return 0;
  } catch (const std::exception& e) {
    Console::Error(e.what());
    return 1;
  } catch (...) {
    Console::Error("Unknown exception");
    return 1;
  }
}
