// cost_probe: sample the chiplet cost model for tools/mip_seed.py.
//
//   cost_probe dump   <design_dir> <tech>
//       blocks (netlist order, areas/powers scaled to <tech>), IO cell areas
//       and the non-zero block-to-block adjacency entries
//   cost_probe eval   <design_dir> <tech> <partition_file>
//       exact cost of a partition (square chiplets) and its decomposition
//   cost_probe fcurve <design_dir> <tech> <lo> <hi> <step> <ar1,ar2,...>
//       per-chiplet cost f(W, ar) of an isolated chiplet of area W
//   cost_probe gcurve <design_dir> <tech> <K> <lo> <hi> <step>
//       interposer/assembly cost of K equal chiplets of area A (no cut)
//   cost_probe ycurve <design_dir> <tech> <K> <A> <io_lo> <io_hi> <io_step>
//       assembly yield of K chiplets with io signals between neighbours
//
// <design_dir> must contain the seven ChipletPart input files with their
// standard names (io_definitions.xml, ..., block_definitions.txt).
#include "cost_model/evaluator_cpp.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

struct Design {
  LibraryDicts* lib = nullptr;
  std::vector<block> blocks;
};

Design load(const std::string& d) {
  Design des;
  des.lib = readLibraries(d + "/io_definitions.xml", d + "/layer_definitions.xml",
                          d + "/wafer_process_definitions.xml",
                          d + "/assembly_process_definitions.xml", d + "/test_definitions.xml",
                          d + "/block_level_netlist.xml");
  if (des.lib) {
    des.blocks = alignBlocksToNetlist(readBlocks(d + "/block_definitions.txt"), des.lib->block_names);
  }
  return des;
}

std::shared_ptr<design::Chip> build(const std::vector<int>& part, const std::string& tech,
                                    float aspect_ratio, LibraryDicts* lib,
                                    const std::vector<block>& blocks) {
  int k = 0;
  for (int p : part) k = std::max(k, p + 1);
  std::vector<std::string> techs(k, tech);
  std::vector<float> ar(k, aspect_ratio), x(k, 0.0f), y(k, 0.0f);
  return buildModel(part, techs, ar, x, y, lib, blocks, false);
}

void zeroAdjacency(LibraryDicts* lib) {
  for (auto& [t, m] : lib->global_adjacency_matrix)
    for (auto& r : m) std::fill(r.begin(), r.end(), 0);
}

// Blank copy of the blocks for the synthetic curves: zero area/power and the
// target tech, so that the areas we assign below are used unscaled by buildModel.
std::vector<block> blankBlocks(const std::vector<block>& blocks, const std::string& tech) {
  std::vector<block> b = blocks;
  for (auto& x : b) {
    x.area = 0.0f;
    x.power = 0.0f;
    x.tech = tech;
  }
  return b;
}

void decompose(const std::shared_ptr<design::Chip>& chip) {
  printf("total %.6f\n", chip->ComputeCost());
  double sum_child = 0;
  for (auto& c : chip->GetChips()) {
    const double cc = c->ComputeCost();
    sum_child += cc;
    printf("child %s core=%.4f io=%.4f area=%.4f cost=%.6f\n", c->GetName().c_str(),
           c->GetCoreArea(), c->GetIoArea(), c->GetArea(), cc);
  }
  double asm_cost = 0;
  if (chip->GetAssemblyProcess())
    asm_cost = chip->GetAssemblyProcess()->AssemblyCost(chip->GetChipsLen(), chip->GetStackedDieArea());
  printf("root self=%.6f children=%.6f assembly=%.6f yield=%.6f signals=%d stacked=%.4f\n",
         chip->GetSelfCost(), sum_child, asm_cost, chip->GetChipTestYield(),
         static_cast<int>(chip->GetChipsSignalCount()), chip->GetStackedDieArea());
}

std::vector<float> parseList(const std::string& s) {
  std::vector<float> out;
  size_t p = 0;
  while (p < s.size()) {
    size_t q = s.find(',', p);
    if (q == std::string::npos) q = s.size();
    out.push_back(std::atof(s.substr(p, q - p).c_str()));
    p = q + 1;
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: cost_probe <dump|eval|fcurve|gcurve|ycurve> <design_dir> <tech> ...\n");
    return 1;
  }
  const std::string mode = argv[1], dir = argv[2], tech = argv[3];
  Design des = load(dir);
  if (!des.lib || des.blocks.empty()) {
    fprintf(stderr, "cost_probe: could not load %s\n", dir.c_str());
    return 1;
  }
  const int n = static_cast<int>(des.blocks.size());

  if (mode == "dump") {
    for (int i = 0; i < n; ++i) {
      const block& b = des.blocks[i];
      printf("block %d %s %.6f %.6f\n", i, b.name.c_str(),
             b.area * area_scaling_factor(b.tech, tech, b.is_memory),
             b.power * power_scaling_factor(b.tech, tech));
    }
    for (auto& io : des.lib->ios)
      printf("io %s tx=%.6g rx=%.6g reach=%.4g\n", io.GetType().c_str(), io.GetTxArea(),
             io.GetRxArea(), io.GetReach());
    for (auto& [t, m] : des.lib->global_adjacency_matrix)
      for (size_t i = 0; i < m.size(); ++i)
        for (size_t j = 0; j < m[i].size(); ++j)
          if (m[i][j]) printf("adj %s %zu %zu %d\n", t.c_str(), i, j, m[i][j]);
    return 0;
  }
  if (mode == "eval") {
    if (argc < 5) { fprintf(stderr, "eval needs a partition file\n"); return 1; }
    std::ifstream f(argv[4]);
    std::vector<int> part;
    int p;
    while (f >> p) part.push_back(p);
    if (static_cast<int>(part.size()) != n) {
      fprintf(stderr, "partition has %zu ids, netlist has %d blocks\n", part.size(), n);
      return 1;
    }
    decompose(build(part, tech, 1.0f, des.lib, des.blocks));
    return 0;
  }
  if (mode == "fcurve") {
    if (argc < 8) { fprintf(stderr, "fcurve needs lo hi step ar-list\n"); return 1; }
    zeroAdjacency(des.lib);
    const double lo = std::atof(argv[4]), hi = std::atof(argv[5]), step = std::atof(argv[6]);
    const std::vector<float> ars = parseList(argv[7]);
    std::vector<block> b = blankBlocks(des.blocks, tech);
    std::vector<int> part(n, 0);
    part[1] = 1;  // chiplet 0 carries the area; chiplet 1 is a 1 mm^2 dummy
    b[1].area = 1.0f;
    for (double W = lo; W <= hi + 1e-9; W += step) {
      b[0].area = static_cast<float>(W);
      for (float ar : ars) {
        auto chip = build(part, tech, ar, des.lib, b);
        printf("f %.4f %.3f %.6f\n", W, ar, chip->GetChips()[0]->ComputeCost());
      }
    }
    return 0;
  }
  if (mode == "gcurve") {
    if (argc < 8) { fprintf(stderr, "gcurve needs K lo hi step\n"); return 1; }
    zeroAdjacency(des.lib);
    const int K = std::atoi(argv[4]);
    const double lo = std::atof(argv[5]), hi = std::atof(argv[6]), step = std::atof(argv[7]);
    std::vector<block> b = blankBlocks(des.blocks, tech);
    std::vector<int> part(n, 0);
    for (int k = 0; k < K && k < n; ++k) part[k] = k;
    for (double A = lo; A <= hi + 1e-9; A += step) {
      for (int k = 0; k < K && k < n; ++k) b[k].area = static_cast<float>(A);
      auto chip = build(part, tech, 1.0f, des.lib, b);
      const double asm_cost =
          chip->GetAssemblyProcess()->AssemblyCost(chip->GetChipsLen(), chip->GetStackedDieArea());
      printf("g %d %.4f %.6f %.6f %.6f %.6f\n", K, A, chip->GetStackedDieArea(), chip->GetSelfCost(),
             asm_cost, chip->GetChipTestYield());
    }
    return 0;
  }
  if (mode == "ycurve") {
    if (argc < 9) { fprintf(stderr, "ycurve needs K A io_lo io_hi io_step\n"); return 1; }
    const int K = std::atoi(argv[4]);
    const double A = std::atof(argv[5]);
    const int lo = std::atoi(argv[6]), hi = std::atoi(argv[7]), step = std::atoi(argv[8]);
    for (int io = lo; io <= hi; io += step) {
      // The collapsed adjacency is cached per library, so reload for each point.
      Design d2 = load(dir);
      zeroAdjacency(d2.lib);
      auto& m = d2.lib->global_adjacency_matrix.begin()->second;
      std::vector<block> b = blankBlocks(d2.blocks, tech);
      std::vector<int> part(n, 0);
      for (int k = 0; k < K && k < n; ++k) {
        part[k] = k;
        b[k].area = static_cast<float>(A);
        if (K > 1 && io > 0) m[k][(k + 1) % K] = io;
      }
      auto chip = build(part, tech, 1.0f, d2.lib, b);
      printf("y %d %d %.6f\n", K, static_cast<int>(chip->GetChipsSignalCount()),
             chip->GetChipTestYield());
    }
    return 0;
  }
  fprintf(stderr, "cost_probe: unknown mode %s\n", mode.c_str());
  return 1;
}
