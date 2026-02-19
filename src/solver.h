#pragma once

#include "node.h"
#include <stdexcept>
#include <vector>

namespace tsp_puzzle {
// NOTE: use this later for the optimal tour and paths to
// differentiate between them:
using NodeId = int;

struct Path {
  std::vector<NodeId> nodes;
  explicit Path(std::vector<NodeId> v) : nodes(std::move(v)) {
    if (nodes.size() < 2)
      throw std::invalid_argument("Path needs >=2 nodes.");
    if (nodes.front() == nodes.back())
      throw std::invalid_argument(
          "Path must not be closed loop. (front!=back)");
  }
  size_t size() const { return nodes.size(); }
};

struct Cycle {
  std::vector<NodeId> nodes;
  explicit Cycle(std::vector<NodeId> v) : nodes(std::move(v)) {
    if (nodes.size() < 3)
      throw std::invalid_argument("Cycle needs >=3 nodes.");
    if (nodes.front() == nodes.back())
      throw std::invalid_argument(
          "Store cycles without the repeated last node.");
  }
  size_t size() const { return nodes.size(); }
};

class Solver {

public:
  std::vector<std::vector<double>> buildD(const std::vector<Node> &nodes);
  // static std::vector<std::vector<int>>
  // computeAllFromStart(const std::vector<std::vector<double>> &D, int s);
  //std::vector<int> optimal_tour(const std::vector<std::vector<double>> &D);
  static double pathLength(const std::vector<std::vector<double>> &D,
                           const std::vector<int> &path);
  static double tourLength(const std::vector<std::vector<double>> &D,
                           const std::vector<int> &tour);
  std::vector<int>
  constrained_optimal_tour_mip(const std::vector<std::vector<double>> &D, int s,
                               int t);
  std::vector<std::vector<std::vector<int>>>
  batch_constrained_optimal_tour_mip(const std::vector<std::vector<double>> &D);
  std::vector<int> optimal_tour_mip(const std::vector<std::vector<double>> &D);
  //double distance(double *x, double *y, int i, int j);
  static void findsubtour(int n, double **sol, int *tourlenP, int *tour);
  bool sanitize_path(std::vector<int> &v, const int &a, const int &b);
};
} // namespace tsp_puzzle
