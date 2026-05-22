# Incremental Puzzle Generator

This is a C++ command line tool for generating incremental puzzles using Voronoi diagrams and integer programming. But what is an Incremental Puzzle? It is a traveling salesperson based puzzle, where the player is asked to solve a series of increasingly more challenging traveling salesperson instances. Given a set of seed nodes, the generator incrementally builds a puzzle by adding nodes one at a time, using [Gurobi](https://www.gurobi.com/) to solve the underlying optimization problem at each step. Output is rendered as SVG.

## Table of Contents

- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
- [Configuration](#configuration)
- [Batch Generation](#batch-generation)
- [Project Structure](#project-structure)

---

## Requirements

- **C++20** compiler
- **CMake** ≥ 3.20
- **Gurobi** (requires a valid license)

On Windows, the project has been tested with **Visual Studio 2022** using the **ClangCL** toolset. On macOS/Linux, a standard Clang works.

---

## Building

### Windows (Visual Studio 2022 + ClangCL)

```bat
cmake -B build -G "Visual Studio 17 2022" -T ClangCL
cmake --build build
```

The compiled binary will be at `.\build\Debug\inc_puzzle_gen.exe`.

### macOS / Linux

```bash
cmake -B build
cmake --build build
```

The compiled binary will be at `./build/inc_puzzle_gen`.

---

## Usage

The generator takes two positional arguments: a CSV file of starting nodes and a TOML configuration file.

```
inc_puzzle_gen <nodes_csv> <config_toml>
```

**Windows example:**

```bat
.\build\Debug\inc_puzzle_gen.exe .\nodes\presentationseed.csv .\puzzle_config.toml
```

**macOS  example:**

```bash
./build/inc_puzzle_gen ./nodes/presentationseed.csv ./puzzle_config.toml
```

### Node CSV format

The CSV file defines the initial seed nodes for the puzzle. Each row should contain the coordinates of one 2d-node (Internally every node is between [-1,1], anything else will be scaled accordingly. Check the example files in `nodes/` for reference).

### Output

By default the generator writes SVG output to disk. If `render_voronoi = true` in the config, a Voronoi diagram of the last state is also rendered.

---

## Configuration

All generation parameters are controlled via `puzzle_config.toml`. A full example:

```toml
delta = 0.075
instance_branch = 1
lookahead_depths = 1
voronoi_resolution = 1000
render_voronoi = true
verbose = false
min_edge_change = 1
total_nodes = 4
svg_viewport_size = 265
epsilon = 0
winding_length_percent = 0.052
benchmark = false
benchmark_time = 1
seed = 34
svg_template = false
```

| Parameter | Description |
|---|---|
| `delta` | Difference between the best and second best solution. |
| `epsilon` | Difference between the solution of two consecutive instances.  . |
| `instance_branch` | Number of branches to explore per step. |
| `lookahead_depths` | How many steps ahead the solver looks when choosing the next node. |
| `voronoi_resolution` | Resolution of the Voronoi diagram (bitmap) rendering (pixels). |
| `render_voronoi` | Whether to render a Voronoi diagram at each step (`true`/`false`). |
| `verbose` | Print some extra information to stdout (`true`/`false`). |
| `min_edge_change` | Minimum number of edges that must change between instances. |
| `total_nodes` | Target total number of nodes in the finished puzzle. |
| `svg_viewport_size` | Width/height of the SVG viewport in pixels. |
| `winding_length_percent` | Loss of string due to winding. |
| `benchmark` | Run in benchmark mode (`true`/`false`). |
| `benchmark_time` | Time limit (seconds) for benchmark runs. |
| `seed` | Random seed for reproducibility. |
| `svg_template` | Output an SVG template(`true`/`false`). |

---

## Batch Generation

`batch_ip_generator.py` automates running the generator across a sweep of parameter combinations and served as a testing tool. Edit the parameter lists at the top of the script (e.g. `seed`, `epsilon`, `delta`) and then run:

```bash
python batch_ip_generator.py
```

The script writes a temporary `config_temp.toml` for each combination, calls the compiled binary, and cleans up afterward. Adjust the path to the binary inside the script if you're on macOS/Linux (currently points to `.\build\Debug\inc_puzzle_gen.exe`).

---

## Project Structure

```
.
├── src/
│   ├── main.cpp           # Entry point
│   ├── solver.cpp / .h    # Gurobi-based IP solver
│   ├── instance.cpp / .h  # Puzzle instance representation
│   ├── options.cpp / .h   # TOML config parsing
│   ├── benchmark.cpp / .h # Benchmarking utilities
│   └── vorosketch-037.cpp # Voronoi diagram renderer
├── nodes/                 # Example seed node CSV files
├── CMakeLists.txt
├── FindGUROBI.cmake       # CMake module for locating Gurobi
├── puzzle_config.toml     # Default configuration
└── batch_ip_generator.py  # Batch runner script
```
