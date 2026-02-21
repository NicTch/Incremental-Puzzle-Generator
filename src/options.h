#pragma once
#include <string>
#include <filesystem>

struct Options {
  double delta = 0.2;
  int n_initial_nodes = 3;
  int instance_branch = 3;
  int lookahead_depths = 3;
  bool runtime_optimization = false;
  int n_candidates = 50;
  int voronoi_resolution = 800;
  bool render_voronoi = false;
  bool verbose = false;
  int min_edge_change = 1;
  int total_nodes = 5;
  int svg_viewport_size = 265;
  double epsilon = 0.2;
  double winding_length_percent = 0.01;
  bool benchmark = false;
  int benchmark_time = 600;
  int seed = 100;

  //output file path
  std::filesystem::path output_path = "";

  // Returns false if the file can't be opened; true otherwise (bad values are
  // ignored).
  static bool parse(Options &out, const std::string &path);
  void save_config(const std::string& sourcePath);
};
