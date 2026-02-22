#pragma once

#include "candidate.h"
#include "decommission.h"
#include "node.h"
#include "options.h"
#include <istream>
#include <memory.h>
#include <set>
#include <vector>

namespace tsp_puzzle {

class Instance {
public:
  std::vector<Node> nodes{};
  std::vector<Decommission> decommissions{};
  std::vector<Candidate> candidates{};
  std::vector<int> optimal_tour{};
  double optimal_tour_length{};
  static std::vector<Node> readNodesCSV(std::istream &in);
  void lookaheadPickMax(int branching, int depth, Options &opt);
  void applyChildToRoot();
  static void scaleToCanvas(std::vector<Node> &nodes);
  int solution_change_cpp(const std::vector<int> &sol_old_cycle,
                          const std::vector<int> &sol_new_path);
  int solution_change_t2t(const std::vector<int> &sol_old_cycle,
                          const std::vector<int> &sol_new_cycle);

  Instance *parent{nullptr};
  std::vector<std::unique_ptr<Instance>> children{};
  Candidate
      fromCandidate{}; // the candidate used to create *this* from its parent
  int edge_change_from_previous{-1};
  int score{0};      // cached cumulative score from here down
  int bestChild{-1}; // index into `children` of the best continuation

  explicit Instance(std::vector<Node> nodesIn, Options &opt);
  std::pair<int, int> normalizePair(int a, int b) const;
  std::set<std::pair<int, int>> cycleEdges(const std::vector<int> &cyc) const;

private:
  
  std::vector<std::pair<int, int>> pathEdges(const std::vector<int> &path);
  // read and cleanup the nodes
  std::vector<Node> initNodes(std::vector<Node> nodes);
  // construct decommissions from nodes
  std::vector<Decommission> initDecommissions(Options &opt);
  // construct candidates from decommissions
  std::vector<Candidate> initCandidate(Options &opt);
  inline double edgeChange(const Candidate &c) const {
    return decommissions[c.nearest].edge_change; // nearest is an index
  }
  void buildSubtree(int branching, int depth, Options &opt);
  void evalScore(int depth, Options &opt);
};
void printDecommissions(const tsp_puzzle::Instance &inst, const int k, Options &opt);
void printCandidates(const tsp_puzzle::Instance &inst, const int k, Options &opt);
void printNodes(const tsp_puzzle::Instance &inst, Options &opt);
void printOPT(const tsp_puzzle::Instance &inst, Options &opt);
void printHammiltomPath(const tsp_puzzle::Instance &inst, const int i,
                        const int j, Options &opt);
std::string generate_svg_with_template(const std::vector<Node> &nodes, const Options &opt,
                         const double radius);
std::string generate_nodes_svg(const std::vector<Node> &nodes, const Options &opt,
                         const double radius);
double summed_angle_percent(const std::vector<int> solution,
                            const std::vector<Node> &nodes);
void print_all_solutions(const tsp_puzzle::Instance &inst, const Options &opt);
} // namespace tsp_puzzle
