#include "benchmark.h"
#include "instance.h"
#include "options.h"
#include "solver.h"
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

namespace tsp_puzzle {

std::vector<Node> random_nodes(const std::vector<Node> &nodes, Options opt) {
  static std::default_random_engine re(std::random_device{}()); // seeded once
  std::uniform_real_distribution<double> unif(-1.0, 1.0);

  std::vector<Node> benchmark_nodes = nodes;
  benchmark_nodes.reserve(opt.total_nodes);

  int node_size = static_cast<int>(nodes.size());
  for (int i = node_size; i < opt.total_nodes; ++i) {
    Node new_node;
    new_node.x = unif(re);
    new_node.y = unif(re);
    new_node.idx = i;
    benchmark_nodes.push_back(new_node);
  }

  return benchmark_nodes;
}

std::vector<Node> generate_random_benchmark(std::vector<Node> nodes,
                                            Options opt) {

  int best_score = std::numeric_limits<int>::min();
  std::vector<Node> current_best;
  std::vector<Node> tmp_nodes;

  auto start = std::chrono::steady_clock::now();
  auto time_limit = std::chrono::seconds(opt.benchmark_time);
  // while (std::chrono::steady_clock::now() - start < time_limit) {
  //   tmp_nodes = random_nodes(nodes, opt);
  //   int score = score_nodes(tmp_nodes, opt);
  //   if (score > best_score || current_best.empty()) {
  //     best_score = score;
  //     current_best = std::move(tmp_nodes);
  //   }
  // }

  return current_best;
}

void print_benchmark(std::vector<Node> nodes, Options opt, int n) {
  std::vector<std::vector<int>> stats;
  std::cout << "Benchmark nodes:\n";
  for (int i = 0; i < n; ++i) {
    auto tmp_nodes = random_nodes(nodes, opt);
    // for (int j = 0; j < tmp_nodes.size(); ++j) {
    //   std::cout << tmp_nodes[j].idx << "," << tmp_nodes[j].x << ","
    //             << tmp_nodes[j].y << "\n";
    // }
    stats.push_back(score_nodes(tmp_nodes, opt));
  }

  std::cout << "Benchmark change:\n";
  for (int i = 0; i < stats.size(); ++i) {
    std::cout << "[";
    for (int j = 0; j < stats[i].size(); ++j) {
      std::cout << stats[i][j] << ",";
    }
    std::cout << "], \n";
  }
  std::cout << "Benchmark change mean:\n";
  std::vector<double> mean;
  mean.assign(stats[0].size(), 0.0);
  for (int i = 0; i < stats.size(); ++i) {
    for (int j = 0; j < stats[i].size(); ++j) {
      mean[j] += stats[i][j];
    }
  }
  for (int i = 0; i < mean.size(); ++i) {
    mean[i] = mean[i] / n;
  }
  std::cout << "[";
  for (int i = 0; i < mean.size(); ++i) {
    std::cout << mean[i] << ",";
  }
  std::cout << "], \n";
}

std::pair<int, int> normalizePair(int a, int b) {
  if (a > b)
    std::swap(a, b);
  return {a, b};
}
std::set<std::pair<int, int>> cycleEdges(const std::vector<int> &cyc) {
  std::set<std::pair<int, int>> S;
  if (cyc.empty())
    return S;
  int n = (int)cyc.size();
  for (int i = 0; i < n; i++) {
    int a = cyc[i], b = cyc[(i + 1) % n];
    S.insert(normalizePair(a, b));
  }
  return S;
}
// Convert a path [v0..vk] to undirected edge list without wrap
std::vector<std::pair<int, int>> pathEdges(const std::vector<int> &path) {
  std::vector<std::pair<int, int>> E;
  if (path.size() < 2)
    return E;
  E.reserve(path.size() - 1);
  for (size_t i = 0; i + 1 < path.size(); ++i) {
    E.push_back(normalizePair(path[i], path[i + 1]));
  }
  return E;
}

// nt solution_change
int solution_change_cpp(const std::vector<int> &sol_old_cycle,
                        const std::vector<int> &sol_new_path) {
  auto set_old = cycleEdges(sol_old_cycle);
  auto Enew = cycleEdges(sol_new_path);
  int diff = 0;
  for (const auto &e : Enew) {
    if (!set_old.count(e))
      ++diff;
  }
  return diff;
}

std::vector<int> score_nodes(const std::vector<Node> &nodes, Options opt) {
  const int N = static_cast<int>(nodes.size());

  Solver solver;
  std::vector<int> total_score;
  total_score.reserve(N - 3);

  std::vector<Node> prefix;
  prefix.reserve(N);
  prefix.push_back(nodes[0]);
  prefix.push_back(nodes[1]);
  prefix.push_back(nodes[2]);
  std::vector<Node> small_inst = prefix;

  auto D_prev = solver.buildD(prefix);
  std::vector<int> tour_prev = solver.optimal_tour_mip(D_prev);

  for (int n = 3; n < N; ++n) {
    // prefix now becomes first n+1 points
    prefix.push_back(nodes[n]);
    auto D_curr = solver.buildD(prefix);
    std::vector<int> tour_curr = solver.optimal_tour_mip(D_curr);

    // for (int i = 0; i < tour_prev.size(); ++i) {
    //   std::cout << tour_prev[i];
    // }
    // std::cout << "\n";
    // for (int i = 0; i < tour_curr.size(); ++i) {
    //   std::cout << tour_curr[i];
    // }
    // std::cout << "\n";

    int change = solution_change_cpp(tour_prev, tour_curr) - 2;
    total_score.push_back(change);

    tour_prev.swap(tour_curr);
  }

  return total_score;
}

} // namespace tsp_puzzle
