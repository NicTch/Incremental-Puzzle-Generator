#pragma once

namespace tsp_puzzle {
class Candidate {
public:
  int nearest{};
  int second{};
  double x{0.0};
  double y{0.0};
};
} // namespace tsp_puzzle
