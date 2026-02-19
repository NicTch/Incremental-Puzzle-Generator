#pragma once

namespace tsp_puzzle {
class Decommission {
public:
  int node1{-1};
  int node2{-1};
  double tsp_tour_length{0.0};
  int edge_change{0};

private:
  // static const int m_idx{0};
};

} // namespace tsp_puzzle
