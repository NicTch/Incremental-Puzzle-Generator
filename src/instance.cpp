#include "instance.h"
#include "options.h"
#include "solver.h"
#include "vorosketch-037.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <fstream>

namespace tsp_puzzle {

Instance::Instance(std::vector<Node> nodesIn, Options &opt) {
  nodes = initNodes(nodesIn);
  decommissions = initDecommissions();
  candidates = initCandidate(opt);
}

std::vector<Node> Instance::initNodes(std::vector<Node> nodes) {
  std::vector<Node> n(std::move(nodes));
  return n;
}

std::vector<Node> Instance::readNodesCSV(std::istream &in) {
  std::vector<Node> nodes;
  std::string line;

  int autoId = 0;
  while (std::getline(in, line)) {
    // strip comments
    const auto hashPos = line.find('#');
    if (hashPos != std::string::npos)
      line.resize(hashPos);

    // skip empty/whitespace only lines
    if (line.find_first_not_of(" \t\r\n") == std::string::npos)
      continue;

    // split by comma
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ','))
      parts.push_back(cell);

    if (parts.size() == 2) {
      // x,y
      Node n;
      n.idx = autoId++;
      n.x = std::stod(parts[0]);
      n.y = std::stod(parts[1]);
      nodes.push_back(n);
    } else {
      std::cerr << "Error: Each node need exactly x and y.\n";
    }
  }
  // if there are no nodes, add [-1,-1]
  if (nodes.empty()) {
    Node n;
    n.idx = autoId++;
    n.x = -1.0;
    n.y = -1.0;
    nodes.push_back(n);
  }
  // if there are less than 4 node in total, 
  //add more nodes where x and y are betweeen [-1,1] untile there are 4 nodes
  while (nodes.size() < 4) {
    Node n;
    n.idx = autoId++;
    n.x = (double)(rand() % 1000) / 1000.0 * 2.0 - 1.0;
    n.y = (double)(rand() % 1000) / 1000.0 * 2.0 - 1.0;
    nodes.push_back(n);
  }
  return nodes;
}

void Instance::scaleToCanvas(std::vector<Node> &nodes) {
  if (nodes.empty())
    return;

  double minx = nodes[0].x, maxx = nodes[0].x;
  double miny = nodes[0].y, maxy = nodes[0].y;
  for (const auto &p : nodes) {
    minx = std::min(minx, p.x);
    maxx = std::max(maxx, p.x);
    miny = std::min(miny, p.y);
    maxy = std::max(maxy, p.y);
  }

  if (minx > -1.001 && maxx < 1.001 && miny > -1.001 && maxy < 1.001) {
    return;
  }

  const double cx = 0.5 * (minx + maxx); // center x
  const double cy = 0.5 * (miny + maxy); // center y
  const double rx = 0.5 * (maxx - minx); // half width
  const double ry = 0.5 * (maxy - miny); // half height
  const double s = std::max(rx, ry);     // uniform scale

  for (auto &p : nodes) {
    p.x = (p.x - cx) / s;
    p.y = (p.y - cy) / s;
  }
}
std::vector<Decommission> Instance::initDecommissions() {
  // INFO: TIMER
  // auto t1 = std::chrono::high_resolution_clock::now();
  const int n = static_cast<int>(nodes.size());
  if (n < 2)
    throw std::runtime_error("Need at least 2 nodes");

  std::vector<Decommission> decommissions;

  Solver solver;
  auto D = solver.buildD(nodes);
  const auto baseTour = solver.optimal_tour_mip(D);
  // saves the optimal tour and its length
  optimal_tour = baseTour;
  optimal_tour_length = solver.tourLength(D, baseTour);
  auto all_tours = solver.batch_constrained_optimal_tour_mip(D);
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      Decommission d;
      // std::vector<int> path = solver.constrained_optimal_tour_mip(D, i, j);
      std::vector<int> path = all_tours[i][j];
      d.node1 = i;
      d.node2 = j;
      d.tsp_tour_length = solver.pathLength(D, path);
      d.edge_change = Instance::solution_change_cpp(baseTour, path);
      d.opt_path = path;
      decommissions.push_back(d);
      // INFO:
      // std::cout << "path of " << i << "," << j << ": [";
      // for (int x : path)
      //   std::cout << x << ' ';
      // std::cout << "]";
      // std::cout << "path legth: " << d.tsp_tour_length << '\n';
    }
  }
  // INFO:
  // std::cout << "OPT: [";
  // for (int x : baseTour)
  //   std::cout << x << ' ';
  // std::cout << "]";
  // std::cout << '\n';

  // INFO: timer
  //  auto t2 = std::chrono::high_resolution_clock::now();
  //  auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 -
  //  t1); std::cout << "InitDecommissions took : " << ms_int.count() << "ms\n";
  return decommissions;
}

std::pair<int, int> Instance::normalizePair(int a, int b) const {
  if (a > b)
    std::swap(a, b);
  return {a, b};
}
std::set<std::pair<int, int>>
Instance::cycleEdges(const std::vector<int> &cyc) const {
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
std::vector<std::pair<int, int>>
Instance::pathEdges(const std::vector<int> &path) {
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
int Instance::solution_change_cpp(const std::vector<int> &sol_old_cycle,
                                  const std::vector<int> &sol_new_path) {
  auto set_old = cycleEdges(sol_old_cycle);
  auto Enew = pathEdges(sol_new_path);
  int diff = 0;
  for (const auto &e : Enew) {
    if (!set_old.count(e))
      ++diff;
  }
  return diff;
}

int Instance::solution_change_t2t(const std::vector<int> &sol_old_cycle,
                                  const std::vector<int> &sol_new_cycle) {
  auto set_old = cycleEdges(sol_old_cycle);
  auto Enew = cycleEdges(sol_new_cycle);
  int diff = -2;
  for (const auto &e : Enew) {
    if (!set_old.count(e))
      ++diff;
  }
  return diff;
}
std::vector<Candidate> Instance::initCandidate(Options &opt) {
  // auto t1 = std::chrono::high_resolution_clock::now();
  std::vector<Candidate> candidates;
  candidates =
      vorosketch::vorosketch_main(nodes, decommissions, opt.voronoi_resolution,
                                  opt.render_voronoi, opt.delta, opt.output_path);
  // auto t2 = std::chrono::high_resolution_clock::now();
  // auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 -
  // t1); std::cout << "InitCandidate took : " << ms_int.count() << "ms\n";
  return candidates;
}

void printDecommissions(const tsp_puzzle::Instance &inst, const int k, Options &opt) {
  std::filesystem::path fullPath = opt.output_path / "log.txt";
  std::ofstream out(fullPath, std::ios::app);

  const auto n = std::min<size_t>(k, inst.decommissions.size());

  std::cout << "Total decommissions: " << inst.decommissions.size() << "\n";
  if (out) out << "Total decommissions: " << inst.decommissions.size() << "\n";

  std::cout << "Decommissions (first " << n << "):\n";
  if (out) out << "Decommissions (first " << n << "):\n";

  for (size_t i = 0; i < n; ++i) {
      const auto &d = inst.decommissions[i];
      std::stringstream ss;
      ss << "[" << i << "] "
          << "node1=" << d.node1 << ", "
          << "node2=" << d.node2 << ", "
          << "tsp_tour_length=" << d.tsp_tour_length << ", "
          << "edge_change=" << d.edge_change << "\n";
      
      std::cout << ss.str();
      if (out) out << ss.str();
  }
}

void printCandidates(const tsp_puzzle::Instance &inst, const int k, Options &opt) {
  std::filesystem::path fullPath = opt.output_path / "log.txt";
  std::ofstream out(fullPath, std::ios::app);

  std::cout << "Total candidates: " << inst.candidates.size() << "\n";
  if (out) out << "Total candidates: " << inst.candidates.size() << "\n";
  const auto n = std::min<size_t>(k, inst.candidates.size());
  std::cout << "Candidates (first " << n << "):\n";
  if (out) out << "Candidates (first " << n << "):\n";

  for (size_t i = 0; i < n; ++i) {
    const auto &c = inst.candidates[i];
    std::stringstream ss;
    ss << "[" << i << "] "
        << "nearest=" << c.nearest << ", "
        << "second=" << c.second << ", "
        << "x=" << c.x << ", "
        << "y=" << c.y << "\n";
    std::cout << ss.str();
    if (out) out << ss.str();
  }
}
void printNodes(const tsp_puzzle::Instance &inst, Options &opt) {
  std::filesystem::path fullPath = opt.output_path / "log.txt";
  std::ofstream out(fullPath, std::ios::app);

  std::cout << "Total nodes: " << inst.nodes.size() << "\n";
  if (out) out << "Total nodes: " << inst.nodes.size() << "\n";
  const auto s = inst.nodes.size();
  for (int i = 0; i < s; ++i) {
    const auto &n = inst.nodes[i];
    std::stringstream ss;
    ss << "\"" << n.idx << "\":"
              << "(" << n.x << ", " << n.y << "),\n";
    std::cout << ss.str();
    if (out) out << ss.str();
  }
}

void printOPT(const tsp_puzzle::Instance &inst, Options &opt) {
  std::filesystem::path fullPath = opt.output_path / "log.txt";
  std::ofstream out(fullPath, std::ios::app);

  Solver solver;
  auto D = solver.buildD(inst.nodes);
  const auto tour = solver.optimal_tour_mip(D);
  const auto s = tour.size();
  std::cout << "Tour: ";
  if (out) out << "Tour: ";
  for (int i = 0; i < s; i++){
    std::stringstream ss;
    ss << tour[i] << " ";
    std::cout << ss.str();
    if (out) out << ss.str();
  }
  std::cout << std::endl;
  if (out) out << std::endl;;
}

void printHammiltomPath(const tsp_puzzle::Instance &inst, const int i,
                        const int j, Options &opt) {
  std::filesystem::path fullPath = opt.output_path / "log.txt";
  std::ofstream out(fullPath, std::ios::app);

  Solver solver;
  auto D = solver.buildD(inst.nodes);
  std::vector<int> path = solver.constrained_optimal_tour_mip(D, i, j);
  const auto s = path.size();
  std::cout << "Path between " + std::to_string(i) + " and " +
                   std::to_string(j) + ": ";
  if (out) out << "Path between " + std::to_string(i) + " and " +
                   std::to_string(j) + ": ";
  for (int i = 0; i < s; i++){
    std::stringstream ss;
    ss << path[i] << " ";
    std::cout << ss.str();
    if (out) out << ss.str();
  }
  std::cout << std::endl;
  if (out) out << std::endl;
}

// static inline bool sameCandidate(const Candidate &a, const Candidate &b) {
//   constexpr double eps = 1e-9;
//   auto eq = [&](double u, double v) { return std::abs(u - v) <= eps; };
//   return a.nearest == b.nearest && a.second == b.second && eq(a.x, b.x) &&
//          eq(a.y, b.y);
// }
void Instance::buildSubtree(int branching, int depth, Options &opt) {
  if (depth <= 0)
    return;

  if (children.empty()) {
    const double min_change = opt.min_edge_change;

    // filter the candidates
    std::vector<Candidate> chosen_cand;
    chosen_cand.reserve(candidates.size());
    for (const auto &c : candidates) {
      // filter candidates having less than min threshhold
      if (decommissions[c.nearest].edge_change < min_change) {
        continue;
      }
      // filter candidates that are not atealst epsilon longer that the old tour
      const double d1x = nodes[decommissions[c.nearest].node1].x - c.x;
      const double d1y = nodes[decommissions[c.nearest].node1].y - c.y;
      const double d2x = nodes[decommissions[c.nearest].node2].x - c.x;
      const double d2y = nodes[decommissions[c.nearest].node2].y - c.y;
      const double detour_length = std::hypot(d1x, d1y) + std::hypot(d2x, d2y);
      const double new_tour_length =
          decommissions[c.nearest].tsp_tour_length + detour_length;
      //std::cout <<"outside epsilon: "<<opt.epsilon << "\n";
      if (std::abs(new_tour_length - optimal_tour_length) < (opt.epsilon) ) {
        //std::cout <<"inside epsilon: "<<opt.epsilon << "\n";
        continue;
      }
      chosen_cand.push_back(c);
    }

    // INFO:
    // {
    //   std::cout << "-----------------BUILDING SUBTREE-----------------\n";
    //   std::cout << "Branch: " << branching << " Depth: " << depth << "\n";
    //   std::cout << "Candidates after filtering: " << chosen_cand.size() <<
    //   "\n";
    // }
    //  INFO:
    // for (auto c : chosen_cand) {
    //   std::cout << "-------------------------------\n";
    //   std::cout << "Candidate:(" << c.x << ", " << c.y << ")\n";
    //   std::cout << " Nearest: [" << c.nearest << "] ";
    //   std::cout << "Second: [" << c.second << "] ";
    //   std::cout << "node1: [" << decommissions[c.nearest].node1 << "] node2:
    //   ["
    //             << decommissions[c.nearest].node2 << "] ";
    //   std::cout << "edgechange of candidate: [";
    //   std::cout << decommissions[c.nearest].edge_change << "]\n";
    // }

    // pick random candidates
    if (chosen_cand.size() <= 0) {
      std::cout << "No candidtates with minimum change " << min_change
                << " for this instace.\n";
      std::cout << "number of core nodes:" << nodes.size() <<"depth: "
                << opt.lookahead_depths - depth <<" \n";
      //printNodes(*this, opt);
    } else if ((int)chosen_cand.size() >= branching) {
      std::mt19937 gen(opt.seed);
      std::shuffle(chosen_cand.begin(), chosen_cand.end(), gen);
      chosen_cand.resize(branching);
    } else {
      std::cout << "There are only " << (int)chosen_cand.size()
                << " viable candidates, instead of " << branching << "\n";
    }
    //  INFO:
    // for (auto c : chosen_cand) {
    //   std::cout << "-------------------------------\n";
    //   std::cout << "Candidate:(" << c.x << ", " << c.y << ")\n";
    //   std::cout << " Nearest: [" << c.nearest << "] ";
    //   std::cout << "Second: [" << c.second << "] ";
    //   std::cout << "node1: [" << decommissions[c.nearest].node1 << "] node2:
    //   ["
    //             << decommissions[c.nearest].node2 << "] ";
    //   std::cout << "edgechange of candidate: [";
    //   std::cout << decommissions[c.nearest].edge_change << "]\n";
    // }
    // INFO: validate the nearest decommission:
    // Node cand;
    // cand.x = chosen_cand[0].x;
    // cand.y = chosen_cand[0].y;
    // const int x = chosen_cand[0].nearest;
    // for (int i = 0; i < decommissions.size(); ++i) {
    //   auto d = decommissions[i];
    //   double tourlength = d.tsp_tour_length;
    //   std::cout << "decom[" << i << "] tourlen with no detour: " <<
    //   tourlength
    //             << "\n";
    //   Node a = nodes[d.node1];
    //   Node b = nodes[d.node2];
    //   double dx1 = a.x - cand.x;
    //   double dy1 = a.y - cand.y;
    //   double dist1 = std::hypot(dx1, dy1);
    //   tourlength += dist1;
    //   double dx2 = b.x - cand.x;
    //   double dy2 = b.y - cand.y;
    //   double dist2 = std::hypot(dx2, dy2);
    //   tourlength += dist2;
    //   std::cout << "decom[" << i << "] tourlen with detour: " << tourlength
    //             << "\n";
    // }

    children.reserve(chosen_cand.size());
    for (const auto &c : chosen_cand) {
      std::vector<Node> childNodes = nodes;
      Node added;
      added.x = c.x;
      added.y = c.y;
      added.idx = (int)childNodes.size();
      childNodes.push_back(added);

      auto child = std::make_unique<Instance>(childNodes, opt);
      child->parent = this;
      child->fromCandidate = c;
      child->edge_change_from_previous = decommissions[c.nearest].edge_change;
      // child->optimal_tour_length = decommissions[c.nearest].tsp_tour_length +
      //                             std::hypot(nodes[decommissions[c.nearest].node1].x - c.x,
      //                                        nodes[decommissions[c.nearest].node1].y - c.y) +
      //                             std::hypot(nodes[decommissions[c.nearest].node2].x - c.x,
      //                                        nodes[decommissions[c.nearest].node2].y - c.y);
      // child->optimal_tour = decommissions[c.nearest].opt_path;
      // child->optimal_tour.push_back(added.idx);
      
      //INFO: print the difference from the prediction and the real tour for each child
      std::cout << "Child candidate: (" << c.x << ", " << c.y << ") edge_change_vor: " 
      << decommissions[c.nearest].edge_change
      << " edge_change_scnd: "
      << decommissions[c.second].edge_change
      << " edge_change_mip: " 
      << Instance::solution_change_t2t(this->optimal_tour, child->optimal_tour)<< "\n";

      std::cout <<"vor optimal tour length: " << decommissions[c.nearest].tsp_tour_length +
       std::hypot(nodes[decommissions[c.nearest].node1].x - c.x, nodes[decommissions[c.nearest].node1].y - c.y) +
       std::hypot(nodes[decommissions[c.nearest].node2].x - c.x, nodes[decommissions[c.nearest].node2].y - c.y)
      << " scnd optimal tour length: " 
       << decommissions[c.second].tsp_tour_length +
       std::hypot(nodes[decommissions[c.second].node1].x - c.x, nodes[decommissions[c.second].node1].y - c.y) +
       std::hypot(nodes[decommissions[c.second].node2].x - c.x, nodes[decommissions[c.second].node2].y - c.y)
       << " minp optimal tour length: "
      << child->optimal_tour_length << "\n";
      std::cout << "vor optimal tour: [";
      for (int x : decommissions[c.nearest].opt_path)
        std::cout << x << ' ';
      std::cout << added.idx << ' ';
      std::cout << "]";
      std::cout << "scnd optimal tour: [";
      for (int x : decommissions[c.second].opt_path)
        std::cout << x << ' ';
      std::cout << added.idx << ' ';
      std::cout << "]";
      std::cout << " minp optimal tour: [";
      for (int x : child->optimal_tour)
        std::cout << x << ' ';
      std::cout << "]\n";

      children.push_back(std::move(child));
    }
  }

  for (auto &ch : children)
    ch->buildSubtree(branching, depth - 1, opt);
}

// Evaluates the Tree from the current location, call buildSubtree() to build
// the tree and populate the children first!
void Instance::evalScore(int depth, Options &opt) {
  if (depth <= 0 || children.empty()) {
    const double min_change = opt.min_edge_change;
    // filter candidates having less than min threshhold
    std::vector<Candidate> filtered_cand;
    filtered_cand.reserve(candidates.size());
    for (const auto &c : candidates) {
      if (decommissions[c.nearest].edge_change >= min_change)
        filtered_cand.push_back(c);
    }

    // NOTE: maybe filter them
    double best_score = -std::numeric_limits<int>::infinity();
    for (auto c : filtered_cand) {
      const int change = decommissions[c.nearest].edge_change;
      if (best_score < change) {
        best_score = change;
      }
    }
    score = best_score;
    bestChild = -1;
    return;
  }

  double current_best_score = -std::numeric_limits<int>::infinity();
  int current_best_child = -1;

  for (int i = 0; i < (int)children.size(); ++i) {
    auto &c = children[i];
    // gives the child a score
    c->evalScore(depth - 1, opt);
    // NOTE:here is how the score is calculated, could use different sum method
    const int new_score = c->score + c->edge_change_from_previous;
    if (new_score > current_best_score) {
      current_best_score = new_score;
      current_best_child = i;
    }
  }

  score = current_best_score;
  bestChild = current_best_child;
  // bestFirst = (idx >= 0) ? children[idx]->fromCandidate : Candidate{};
  return;
}

// Calculates the bestChild member of this Instance
void Instance::lookaheadPickMax(int branching, int depth, Options &opt) {
    buildSubtree(branching, depth, opt);
    evalScore(depth, opt);
}

void Instance::applyChildToRoot() {
  if (bestChild < 0 || bestChild >= (int)children.size()){
    std::cout << "Error: No best child" << "\n";
    return;
  }

  std::unique_ptr<Instance> winner = std::move(children[bestChild]);
  children.clear();

  std::swap(nodes, winner->nodes);
  std::swap(decommissions, winner->decommissions);
  std::swap(candidates, winner->candidates);
  std::swap(children, winner->children);
  std::swap(score, winner->score);
  std::swap(bestChild, winner->bestChild);
  std::swap(edge_change_from_previous, winner->edge_change_from_previous);
  std::swap(optimal_tour, winner->optimal_tour);
  std::swap(optimal_tour_length, winner->optimal_tour_length);
  parent = nullptr;
}

inline double edgeChangeOf(const Instance &ctx, const Candidate &c) {
  return ctx.decommissions[c.nearest].edge_change;
}

std::string nodes_to_svg(const std::vector<Node> &nodes, const Options &opt,
                         const double radius = 0.01) {
  std::ostringstream out;
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 2 2\"";
  out << " width=\"" << opt.svg_viewport_size << "\" height=\""
      << opt.svg_viewport_size << "\"";
  out << ">\n  <g id=\"points\">\n";

  out << std::fixed << std::setprecision(6);
  for (size_t i = 0; i < nodes.size(); ++i) {
    double x = nodes[i].x;
    double y = nodes[i].y;
    // safety check
    if (x < -1.001 || x > 1.001 || y < -1.001 || y > 1.001) {
      std::cout << "SVG export: node out of bounds\n";
      continue;
    }
    // move the frame from [-1,1]x [-1,1] to [0,4]x[0,4]
    // flip Y so (0,0) is bottom-left visually
    double x_svg = x + 1.0;
    double y_svg = 2.0 - (y + 1);
    out << "    <circle id=\"pt-" << i << "\" cx=\"" << x_svg << "\" cy=\""
        << y_svg << "\" r=\"" << radius << "\" fill=\"#000\" />\n";
  }
  // extra point as anchor for the markings if the solution is too long
  {
    double x = 1.0;
    double y = -1.0;
    double x_svg = x + 1.0;
    double y_svg = 2.0 - (y + 1.0);
    out << "    <circle id=\"anchor\" cx=\"" << x_svg << "\" cy=\"" << y_svg
        << "\" r=\"" << radius << "\" fill=\"#000\" />\n";
  }
  out << "  </g>\n";

  // add the solutions
  out << "  <g id=\"circles\">\n";

  Solver solver;
  double longest_tour;
  // go though all the subproblems starting with the full set and go down to 3
  // nodes
  for (int i = nodes.size(); i >= 3; --i) {
    // calculate opt
    std::vector<Node> subset = nodes;
    subset.resize(std::max(0, i));
    const auto D = solver.buildD(subset);
    const auto opt_tour = solver.optimal_tour_mip(D);
    // calculate it's length
    double tour_length = 0.0;
    tour_length = solver.tourLength(D, opt_tour);
    double winding_factor = 0.0;
    winding_factor = summed_angle_percent(opt_tour, nodes);
    double winding_tolerance =
        (2 * opt.winding_length_percent * winding_factor);
    double radius;
    if (i == nodes.size()) {
      longest_tour = tour_length + winding_tolerance;
      radius = 0.01;
    } else {
      radius = longest_tour - (tour_length + winding_tolerance);
    }

    // start drawing the circles
    // set the cetner to bottom left
    double cx = -1.0;
    double cy = -1.0;

    // if radius > 2: new center (2,-2)(bottom right) and radius reduced by 2
    // (to simulate the bend)
    if (radius > 2.0) {
      cx = 1.0;
      cy = -1.0;
      radius = radius - 2.0;
      if (radius <= 0.0) {
        // if we get a negative radius we set it as close to the anchor as
        // possible
        std::cout << "Solution radius error: the radius after the should not "
                     "be negative.";
        radius = 0.1;
      }
    }

    // same transform as for nodes
    double cx_svg = cx + 1.0;
    double cy_svg = 2.0 - (cy + 1.0);

    out << "    <circle id=\"solution-" << i << "\" cx=\"" << cx_svg
        << "\" cy=\"" << cy_svg << "\" r=\"" << radius
        << "\" fill=\"none\" stroke=\"#000\" stroke-width=\"0.01\" />\n";
  }
  out << "  </g>\n</svg>\n";
  return out.str();
}

double summed_angle_percent(const std::vector<int> solution,
                            const std::vector<Node> &nodes) {
  double sum = 0.0;

  for (int i = 0; i < solution.size(); ++i) {
    const int &prev = solution[(i - 1) % solution.size()];
    const int &curr = solution[i];
    const int &next = solution[(i + 1) % solution.size()];
    double ax = nodes[prev].x - nodes[curr].x;
    double ay = nodes[prev].y - nodes[curr].y;
    double bx = nodes[next].x - nodes[curr].x;
    double by = nodes[next].y - nodes[curr].y;

    // calculate angle from dot and cross product
    double dot = ax * bx + ay * by;
    double cross = ax * by - ay * bx;
    double theta = std::atan2(cross, dot);
    // smaller angle [0, pi]
    double small = std::fabs(theta);
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;

    // Larger angle (in (pi, 2pi], except special cases)
    double large = TWO_PI - small;

    // Convert to % of full circle
    double percent = large / TWO_PI;
    sum += percent;
  }
  return sum;
}

void print_all_solutions(const tsp_puzzle::Instance &inst, const Options &opt) {
  std::vector<std::vector<int>> solutions;
  std::vector<double> solution_legth;

  Solver solver;
  for (int i = 3; i <= inst.nodes.size(); ++i) {
    // calculate opt
    std::vector<Node> subset = inst.nodes;
    subset.resize(std::max(0, i));
    const auto D = solver.buildD(subset);
    const auto opt_tour = solver.optimal_tour_mip(D);
    solutions.push_back(opt_tour);
    // calculate it's legth
    double tour_length = 0.0;
    tour_length = solver.tourLength(D, opt_tour);
    solution_legth.push_back(tour_length);
  }

  // print
  std::filesystem::path fullPath = opt.output_path / "log.txt";
  std::ofstream out(fullPath, std::ios::app);

  std::cout << "Solutions:\n";
  for (int i = 0; i < solutions.size(); ++i) {
    std::cout << "[ ";
    if (out) out << "[ ";

    for (int j = 0; j < solutions[i].size(); ++j) {
        std::cout << solutions[i][j] << " ";
        if (out) out << solutions[i][j] << " ";
    }

    std::cout << "] ";
    if (out) out << "] ";

    double scaling_factor = opt.svg_viewport_size / 2.0;
    std::stringstream ss;
    ss << "Scaled length: " << solution_legth[i] * scaling_factor
        << " (Unscaled: " << solution_legth[i] << ")\n";

    std::cout << ss.str();
    if (out) out << ss.str();
  }
  //save the difference between the instances
  if (out) {out << "solution length difference between instances: \n";}
  if (out) {out << "[ ";}
  for (int i = 1; i < solution_legth.size(); ++i){
    if (out) {out << solution_legth[i] - solution_legth[i-1] << ", ";}
  }
  if (out) {out << "] \n";}

  //save the change between insances
  
  if (out) {out << "edge change between insances: ";}
  if (out) {out << "[ ";}
  for (int i = 1; i < solutions.size(); ++i){
    auto set_old = inst.cycleEdges(solutions[i-1]);
    auto Enew = inst.cycleEdges(solutions[i]);
    int diff = -2;
    for (const auto &e : Enew) {
      if (!set_old.count(e))
        ++diff;
      }
    if (out) {out << diff << ", ";}
  }
  if (out) {out << "] \n";}
}
} // namespace tsp_puzzle
