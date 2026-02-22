import subprocess
import os

# Experiment parameters
instance_branch = [5,10,15]
lookahead_depths = [0,1,2]
# min_edge_change = [1]
# epsilon = [0.075]
# delta = [0.075]

trials = 3

base_toml = r"""
delta = 0.075
#instance_branch = 10
#lookahead_depths = 3
voronoi_resolution = 300
render_voronoi = false
verbose = false
min_edge_change = 1
total_nodes = 14
svg_viewport_size = 265
epsilon = 0.075
winding_length_percent = 0.052
benchmark = false
benchmark_time = 1
seed = 43
svg_template = false
"""
for t in range(trials):
    for b in instance_branch:
        for n in lookahead_depths:
            # 1. Create a temporary config file for this run
            config_filename = fr"config_temp.toml"
            with open(config_filename, "w") as f:
                f.write(base_toml)
                print(fr"instance_branch = {b}", file=f)
                print(fr"lookahead_depths = {n}", file=f)

            # 2. Call your C++ executable
            try:
                # Assuming your compiled binary is named 'solver'
                result = subprocess.run(
                    [r".\build\Debug\inc_puzzle_gen.exe", r".\nodes\deltatest.csv", config_filename]
                    #,capture_output=True, text=True
                )
                
                # 3. Save logs or parse output immediately
                # with open(f"results_b{b}_n{n}.log", "w") as log:
                #     log.write(result.stdout)
                    
            except Exception as e:
                print(f"Run failed: {e}")

            # Cleanup temp config if desired
            os.remove(config_filename)