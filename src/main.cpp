#include "benchmark.h"
#include "instance.h"
#include "options.h"
#include "solver.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

std::filesystem::path createTimestampedFolder(const std::string& suffix) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss; 
    ss << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S");

    std::string folderName = ss.str() + "_" + suffix;

    std::filesystem::path baseDir = "output";
    std::filesystem::path targetPath = baseDir / folderName;

    try {
        if (std::filesystem::create_directories(targetPath)) {
            std::cout << "Success: Folder created at " << targetPath << "\n";
        } else {
            std::cout << "Notice: Folder already exists or was not created.\n";
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return targetPath;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: app file.csv config.toml\n";
    return 1;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::cerr << "Could not open " << argv[1] << "\n";
    return 1;
  }

  Options options;
  if (!Options::parse(options, argv[2])) {
    std::cerr << "Use default options." << "\n";
  }

  options.output_path= createTimestampedFolder(std::to_string(options.total_nodes) + "n");

  options.save_config(argv[2]);
  
  if (options.benchmark) {
    auto nodes = tsp_puzzle::Instance::readNodesCSV(in);
    tsp_puzzle::Instance::scaleToCanvas(nodes);
    tsp_puzzle::Instance inst(std::move(nodes), options);
    tsp_puzzle::print_benchmark(inst.nodes, options, 1000);
  } else {
    try {
      // NOTE: TIMER
      auto t1 = std::chrono::high_resolution_clock::now();
      auto nodes = tsp_puzzle::Instance::readNodesCSV(in);
      tsp_puzzle::Instance::scaleToCanvas(nodes);
      tsp_puzzle::Instance inst(std::move(nodes), options);
      auto t2 = std::chrono::high_resolution_clock::now();
      auto ms_int =
          std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
      std::cout << "Time for initial instance: ";
      std::cout << ms_int.count() << "ms\n";

      int nodes_to_add = options.total_nodes - inst.nodes.size();
      for (int i = 0; i < nodes_to_add; ++i) {
        auto t1 = std::chrono::high_resolution_clock::now();
        inst.lookaheadPickMax(options.instance_branch, options.lookahead_depths,
                              options);

        // INFO: look at the best child
        std::cout << "There are " << inst.children.size()
                  << " viable children\n";
        std::cout << "Old_tour_length: " << inst.optimal_tour_length
                  << " New_tour_length: "
                  << inst.children[inst.bestChild]->optimal_tour_length << "\n";
        std::cout << "Length difference to new tour: "
                  << inst.children[inst.bestChild]->optimal_tour_length -
                         inst.optimal_tour_length
                  << "\n";

        inst.applyChildToRoot();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto ms_int =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
        std::cout << i + 1 << ". new node" << " appended!";
        std::cout << "  (Time needed " << ms_int.count() << "ms)\n";
      }

      // TODO: add options
      if (true) {
        std::filesystem::path fullPath = options.output_path / "puzzle.svg";
        std::ofstream out(fullPath);
        if (!out) {
          // error
        } else {
          std::string svg = tsp_puzzle::nodes_to_svg(inst.nodes, options, 0.01);
          out << svg;
        }
      }

      if (options.verbose) {
        std::cout << "-------------verbose---------------\n";
        //tsp_puzzle::printOPT(inst,options);
        // tsp_puzzle::printHammiltomPath(inst, 0, 5);
        //tsp_puzzle::printDecommissions(inst, 10,options);
        //tsp_puzzle::printCandidates(inst, 5,options);
      }
      tsp_puzzle::printNodes(inst,options);
      tsp_puzzle::print_all_solutions(inst, options);

      //std::cout << "Loaded " << inst.nodes.size() << " points\n";
    } catch (const std::exception &e) {
      std::cerr << "Failed to build Instance: " << e.what() << "\n";
      return 1;
    }
  }

  return 0;
}
