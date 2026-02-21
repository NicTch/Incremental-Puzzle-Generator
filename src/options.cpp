#include "options.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>

static std::string trim(std::string s) {
  auto notsp = [](int ch) { return !std::isspace(ch); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), notsp));
  s.erase(std::find_if(s.rbegin(), s.rend(), notsp).base(), s.end());
  return s;
}

static bool streqi(const std::string &a, const std::string &b) {
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
      return false;
  return true;
}

static bool to_bool(const std::string &s, bool &v) {
  if (streqi(s, "true") || streqi(s, "yes") || streqi(s, "on") || s == "1") {
    v = true;
    return true;
  }
  if (streqi(s, "false") || streqi(s, "no") || streqi(s, "off") || s == "0") {
    v = false;
    return true;
  }
  return false;
}

bool Options::parse(Options &out, const std::string &path) {
  std::ifstream in(path.c_str());
  if (!in)
    return false;

  std::string line;
  while (std::getline(in, line)) {
    // strip comments (#, ;, //)
    auto cut = line.find_first_of("#;");
    if (cut != std::string::npos)
      line.resize(cut);
    auto slash = line.find("//");
    if (slash != std::string::npos)
      line.resize(slash);

    line = trim(line);
    if (line.empty() || (line.front() == '[' && line.back() == ']'))
      continue;

    auto eq = line.find('=');
    if (eq == std::string::npos)
      continue;

    std::string key = trim(line.substr(0, eq));
    std::string val = trim(line.substr(eq + 1));

    if (!val.empty() && (val.front() == '"' || val.front() == '\'')) {
      char q = val.front();
      if (val.size() >= 2 && val.back() == q)
        val = val.substr(1, val.size() - 2);
    }

    try {
      if (key == "delta") {
        size_t i = 0;
        double v = std::stod(val, &i);
        if (i == val.size())
        // double it, since we work in the range between [-1,1]
          out.delta = v *2.0;
      } else if (key == "n_initial_nodes") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.n_initial_nodes = v;
      } else if (key == "instance_branch") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.instance_branch = v;
      } else if (key == "lookahead_depths") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.lookahead_depths = v;
      } else if (key == "runtime_optimization") {
        bool v;
        if (to_bool(val, v))
          out.runtime_optimization = v;
      } else if (key == "n_candidates") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.n_candidates = v;
      } else if (key == "voronoi_resolution") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.voronoi_resolution = v;
      } else if (key == "render_voronoi") {
        bool v;
        if (to_bool(val, v))
          out.render_voronoi = v;
      } else if (key == "verbose") {
        bool v;
        if (to_bool(val, v))
          out.verbose = v;
      } else if (key == "min_edge_change") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.min_edge_change = v;
      } else if (key == "total_nodes") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.total_nodes = v;
      } else if (key == "svg_viewport_size") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.svg_viewport_size = v;
      } else if (key == "epsilon") {
        size_t i = 0;
        double v = std::stod(val, &i);
        if (i == val.size())
        // double it, since we work in the range between [-1,1]
          out.epsilon = v *2.0;
      } else if (key == "winding_length_percent") {
        size_t i = 0;
        double v = std::stod(val, &i);
        if (i == val.size())
          out.winding_length_percent = v;
      } else if (key == "benchmark") {
        bool v;
        if (to_bool(val, v))
          out.benchmark = v;
      } else if (key == "benchmark_time") {
        size_t i = 0;
        double v = std::stod(val, &i);
        if (i == val.size())
          out.benchmark_time = v;
      }else if (key == "seed") {
        size_t i = 0;
        int v = std::stoi(val, &i);
        if (i == val.size())
          out.seed = v;
      }
    } catch (...) {
      // ignore bad values; keep defaults
    }
  }
  return true;
}
void Options::save_config(const std::string& sourcePath) {
    if (this->output_path.empty()) 
      return;
    try {
        std::filesystem::path src(sourcePath);
        std::filesystem::path dst = this->output_path / src.filename();
        std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
    } catch (...) {
        // Silently fail or log error as per your preference
    }
}
