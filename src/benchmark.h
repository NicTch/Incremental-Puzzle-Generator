#pragma once

#include "node.h"
#include "options.h"
#include <set>
#include <vector>

namespace tsp_puzzle {

std::vector<Node> random_nodes(const std::vector<Node> &nodes, Options opt);
std::vector<Node> generate_random_benchmark(std::vector<Node> nodes,
                                            Options opt);
std::vector<int> score_nodes(const std::vector<Node> &nodes, Options opt);
void print_benchmark(std::vector<Node> nodes, Options opt, int n);
std::pair<int, int> normalizePair(int a, int b);
std::set<std::pair<int, int>> cycleEdges(const std::vector<int> &cyc);
std::vector<std::pair<int, int>> pathEdges(const std::vector<int> &path);
int solution_change_cpp(const std::vector<int> &sol_old_cycle,
                        const std::vector<int> &sol_new_path);
} // namespace tsp_puzzle
