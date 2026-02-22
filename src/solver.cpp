#include "solver.h"
#include "gurobi_c++.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>

namespace tsp_puzzle {

// euclidean distance between all pairs of nodes
std::vector<std::vector<double>>
Solver::buildD(const std::vector<Node> &nodes) {
  const int n = static_cast<int>(nodes.size());
  std::vector<std::vector<double>> D(n, std::vector<double>(n, 0.0));
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      double dx = nodes[i].x - nodes[j].x;
      double dy = nodes[i].y - nodes[j].y;
      double d = std::hypot(dx, dy);
      D[i][j] = D[j][i] = d;
    }
  }
  return D;
}

double Solver::pathLength(const std::vector<std::vector<double>> &D,
                          const std::vector<int> &path) {
  double sum = 0.0;
  for (size_t i = 1; i < path.size(); ++i) {
    int a = path[i - 1], b = path[i];
    sum += D[a][b];
  }
  return sum;
}
double Solver::tourLength(const std::vector<std::vector<double>> &D,
                          const std::vector<int> &tour) {
  double sum = 0.0;
  int tour_size = tour.size();
  for (size_t i = 1; i <= tour_size; ++i) {
    int a = tour[i - 1], b = tour[i % tour_size];
    sum += D[a][b];
  }
  return sum;
}

class subtourelim : public GRBCallback {
public:
  GRBVar **vars;
  int n;
  subtourelim(GRBVar **xvars, int xn) {
    vars = xvars;
    n = xn;
  }

protected:
  void callback() {
    try {
      if (where == GRB_CB_MIPSOL) {
        // Found an integer feasible solution - does it visit every node?
        double **x = new double *[n];
        int *tour = new int[n];
        int i, j, len;
        for (i = 0; i < n; i++)
          x[i] = getSolution(vars[i], n);

        Solver::findsubtour(n, x, &len, tour);

        if (len < n) {
          // Add subtour elimination constraint
          GRBLinExpr expr = 0;
          for (i = 0; i < len; i++)
            for (j = i + 1; j < len; j++)
              expr += vars[tour[i]][tour[j]];
          addLazy(expr <= len - 1);
        }

        for (i = 0; i < n; i++)
          delete[] x[i];
        delete[] x;
        delete[] tour;
      }
    } catch (GRBException e) {
      std::cout << "Error number: " << e.getErrorCode() << std::endl;
      std::cout << e.getMessage() << std::endl;
    } catch (...) {
      std::cout << "Error during callback" << std::endl;
    }
  }
};

// Given an integer-feasible solution 'sol', find the smallest
// sub-tour.  Result is returned in 'tour', and length is
// returned in 'tourlenP'.

void Solver::findsubtour(int n, double **sol, int *tourlenP, int *tour) {
  bool *seen = new bool[n];
  int bestind, bestlen;
  int i, node, len, start;

  for (i = 0; i < n; i++)
    seen[i] = false;

  start = 0;
  bestlen = n + 1;
  bestind = -1;
  node = 0;
  while (start < n) {
    for (node = 0; node < n; node++)
      if (!seen[node])
        break;
    if (node == n)
      break;
    for (len = 0; len < n; len++) {
      tour[start + len] = node;
      seen[node] = true;
      for (i = 0; i < n; i++) {
        if (sol[node][i] > 0.5 && !seen[i]) {
          node = i;
          break;
        }
      }
      if (i == n) {
        len++;
        if (len < bestlen) {
          bestlen = len;
          bestind = start;
        }
        start += len;
        break;
      }
    }
  }

  for (i = 0; i < bestlen; i++)
    tour[i] = tour[bestind + i];
  *tourlenP = bestlen;

  delete[] seen;
}

std::vector<int>
Solver::optimal_tour_mip(const std::vector<std::vector<double>> &D) {

  const int n = (int)D.size();
  std::vector<int> tour(n);
  double *x = new double[n];
  double *y = new double[n];

  int i;
  // for (i = 0; i < n; i++) {
  //   x[i] = ((double)rand()) / RAND_MAX;
  //   y[i] = ((double)rand()) / RAND_MAX;
  // }

  GRBEnv *env = NULL;
  GRBVar **vars = NULL;

  vars = new GRBVar *[n];
  for (i = 0; i < n; i++)
    vars[i] = new GRBVar[n];

  try {
    int j;

    // normal env
    // env = new GRBEnv();
    // GRBModel model = GRBModel(*env);

    // env with no output
    GRBEnv env = GRBEnv(true);
    env.set(GRB_IntParam_OutputFlag, 0);
    env.start();
    GRBModel model = GRBModel(env);

    // Must set LazyConstraints parameter when using lazy constraints

    model.set(GRB_IntParam_LazyConstraints, 1);

    // Create binary decision variables

    for (i = 0; i < n; i++) {
      for (j = 0; j <= i; j++) {
        vars[i][j] =
            model.addVar(0.0, 1.0, D[i][j], GRB_BINARY,
                         "x_" + std::to_string(i) + "_" + std::to_string(j));
        vars[j][i] = vars[i][j];
      }
    }

    // Degree-2 constraints

    for (i = 0; i < n; i++) {
      GRBLinExpr expr = 0;
      for (j = 0; j < n; j++)
        expr += vars[i][j];
      model.addConstr(expr == 2, "deg2_" + std::to_string(i));
    }

    // Forbid edge from node back to itself

    for (i = 0; i < n; i++)
      vars[i][i].set(GRB_DoubleAttr_UB, 0);

    // Set callback function

    subtourelim cb = subtourelim(vars, n);
    model.setCallback(&cb);

    // Optimize model

    model.optimize();

    // Extract solution

    if (model.get(GRB_IntAttr_SolCount) > 0) {
      double **sol = new double *[n];
      for (i = 0; i < n; i++)
        sol[i] = model.get(GRB_DoubleAttr_X, vars[i], n);

      int len;

      Solver::findsubtour(n, sol, &len, tour.data());
      assert(len == n);

      // std::cout << "Tour: ";
      // for (i = 0; i < len; i++)
      //   std::cout << tour[i] << " ";
      // std::cout << std::endl;

      for (i = 0; i < n; i++)
        delete[] sol[i];
      delete[] sol;
    }

  } catch (GRBException e) {
    std::cout << "Error number: " << e.getErrorCode() << std::endl;
    std::cout << e.getMessage() << std::endl;
  } catch (...) {
    std::cout << "Error during optimization" << std::endl;
  }

  for (i = 0; i < n; i++)
    delete[] vars[i];
  delete[] vars;
  delete[] x;
  delete[] y;
  delete env;
  return tour;
}

std::vector<int>
Solver::constrained_optimal_tour_mip(const std::vector<std::vector<double>> &D,
                                     int s, int t) {

  const int n = (int)D.size();
  std::vector<int> tour(n);
  double *x = new double[n];
  double *y = new double[n];

  int i;
  // for (i = 0; i < n; i++) {
  //   x[i] = ((double)rand()) / RAND_MAX;
  //   y[i] = ((double)rand()) / RAND_MAX;
  // }

  GRBEnv *env = NULL;
  GRBVar **vars = NULL;

  vars = new GRBVar *[n];
  for (i = 0; i < n; i++)
    vars[i] = new GRBVar[n];

  try {
    int j;

    // normal env
    // env = new GRBEnv();
    // GRBModel model = GRBModel(*env);

    // env with no output
    GRBEnv env = GRBEnv(true);
    env.set(GRB_IntParam_OutputFlag, 0);
    env.start();
    GRBModel model = GRBModel(env);

    // Must set LazyConstraints parameter when using lazy constraints

    model.set(GRB_IntParam_LazyConstraints, 1);

    // Create binary decision variables

    for (i = 0; i < n; i++) {
      for (j = 0; j <= i; j++) {
        vars[i][j] =
            model.addVar(0.0, 1.0, D[i][j], GRB_BINARY,
                         "x_" + std::to_string(i) + "_" + std::to_string(j));
        vars[j][i] = vars[i][j];
      }
    }

    // Forced Edge Constraint

    GRBLinExpr expr = vars[s][t];
    model.addConstr(expr == 1,
                    "forced_x_" + std::to_string(s) + "_" + std::to_string(t));

    // Degree-2 constraints

    for (i = 0; i < n; i++) {
      GRBLinExpr expr = 0;
      for (j = 0; j < n; j++)
        expr += vars[i][j];
      model.addConstr(expr == 2, "deg2_" + std::to_string(i));
    }

    // Forbid edge from node back to itself

    for (i = 0; i < n; i++)
      vars[i][i].set(GRB_DoubleAttr_UB, 0);

    // Set callback function

    subtourelim cb = subtourelim(vars, n);
    model.setCallback(&cb);

    // Optimize model

    model.optimize();

    // Extract solution

    if (model.get(GRB_IntAttr_SolCount) > 0) {
      double **sol = new double *[n];
      for (i = 0; i < n; i++)
        sol[i] = model.get(GRB_DoubleAttr_X, vars[i], n);

      int len;

      Solver::findsubtour(n, sol, &len, tour.data());
      assert(len == n);

      // std::cout << "Tour: ";
      // for (i = 0; i < len; i++)
      //   std::cout << tour[i] << " ";
      // std::cout << std::endl;

      for (i = 0; i < n; i++)
        delete[] sol[i];
      delete[] sol;
    }

  } catch (GRBException e) {
    std::cout << "Error number: " << e.getErrorCode() << std::endl;
    std::cout << e.getMessage() << std::endl;
  } catch (...) {
    std::cout << "Error during optimization" << std::endl;
  }

  for (i = 0; i < n; i++)
    delete[] vars[i];
  delete[] vars;
  delete[] x;
  delete[] y;
  delete env;
  return tour;
}

CombinedResult Solver::batch_constrained_optimal_tour_mip(
    const std::vector<std::vector<double>> &D) {

  const int n = static_cast<int>(D.size());

  CombinedResult result;
  result.best_tours.assign(n, std::vector<std::vector<int>>(n));
  result.second_tours.assign(n, std::vector<std::vector<int>>(n));
  result.best_path_lengths.assign(n, std::vector<double>(n, -1.0));
  result.second_path_lengths.assign(n, std::vector<double>(n, -1.0));

  // Basic validation
  if (n < 3) return result;
  for (int i = 0; i < n; ++i) {
    if (static_cast<int>(D[i].size()) != n) {
      throw std::invalid_argument("batch_constrained_optimal_tour_mip: D must be square");
    }
  }

  try {
    GRBEnv env(true);
    env.set(GRB_IntParam_OutputFlag, 0);
    env.start();

    GRBModel model(env);
    model.set(GRB_IntParam_LazyConstraints, 1);

    // x is a symmetric matrix of binary vars (including diagonal, later fixed to 0)
    std::vector<std::vector<GRBVar>> x(n, std::vector<GRBVar>(n));
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j <= i; ++j) {
        x[i][j] = model.addVar(
            0.0, 1.0, D[i][j], GRB_BINARY,
            "x_" + std::to_string(i) + "_" + std::to_string(j));
        x[j][i] = x[i][j]; // mirror handle
      }
    }

    // Degree-2 constraints
    for (int i = 0; i < n; ++i) {
      GRBLinExpr deg = 0;
      for (int j = 0; j < n; ++j) deg += x[i][j];
      model.addConstr(deg == 2, "deg2_" + std::to_string(i));
    }

    // Forbid self-edges
    for (int i = 0; i < n; ++i) x[i][i].set(GRB_DoubleAttr_UB, 0.0);

    // Wire callback (expects GRBVar** with row pointers)
    std::vector<GRBVar *> xrows(n);
    for (int i = 0; i < n; ++i) xrows[i] = x[i].data();
    subtourelim cb(xrows.data(), n);
    model.setCallback(&cb);

    model.update();

    // Helper: extract tour (and optionally list active edges) from current solution.
    auto extractTour = [&](std::vector<int> &tour,
                           std::vector<std::pair<int, int>> *active_edges) -> bool {
      // read x-values
      std::vector<std::vector<double>> xval(n, std::vector<double>(n, 0.0));
      for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
          xval[i][j] = x[i][j].get(GRB_DoubleAttr_X);
        }
      }

      // build row pointers for findsubtour(double**)
      std::vector<double *> xval_rows(n);
      for (int i = 0; i < n; ++i) xval_rows[i] = xval[i].data();

      int len = 0;
      Solver::findsubtour(n, xval_rows.data(), &len, tour.data());
      if (len != n) return false;

      if (active_edges) {
        active_edges->clear();
        active_edges->reserve(n);
        for (int i = 0; i < n; ++i) {
          for (int j = i + 1; j < n; ++j) {
            if (xval[i][j] > 0.5) active_edges->push_back({i, j});
          }
        }
      }
      return true;
    };

    // Force edges via bounds (restore previous forced edge each iteration)
    int prev_i = -1, prev_j = -1;

    for (int i = 0; i < n; ++i) {
      for (int j = i + 1; j < n; ++j) {

        // Unforce previous edge
        if (prev_i != -1) {
          x[prev_i][prev_j].set(GRB_DoubleAttr_LB, 0.0);
          x[prev_i][prev_j].set(GRB_DoubleAttr_UB, 1.0);
        }

        // Force (i,j)
        x[i][j].set(GRB_DoubleAttr_LB, 1.0);
        x[i][j].set(GRB_DoubleAttr_UB, 1.0);
        prev_i = i; prev_j = j;

        model.optimize();
        if (model.get(GRB_IntAttr_SolCount) == 0) continue;

        const double best_obj = model.get(GRB_DoubleAttr_ObjVal);

        std::vector<int> best_tour(n);
        std::vector<std::pair<int, int>> best_edges;
        if (!extractTour(best_tour, &best_edges)) continue;

        if (sanitize_path(best_tour, i, j)) {
          result.best_tours[i][j] = best_tour;
          // path length = tour length - forced edge cost
          result.best_path_lengths[i][j] = best_obj - D[i][j];
        }

        // No-good cut to exclude exactly this edge set
        if (best_edges.empty()) {
          result.second_path_lengths[i][j] = std::numeric_limits<double>::infinity();
          continue;
        }

        GRBLinExpr ban = 0;
        for (const auto &e : best_edges) ban += x[e.first][e.second];

        const int m = static_cast<int>(best_edges.size()); // should be n for a tour
        GRBConstr ban_constr = model.addConstr(
            ban <= m - 1,
            "ban_best_" + std::to_string(i) + "_" + std::to_string(j));
        model.update();

        model.optimize();

        if (model.get(GRB_IntAttr_SolCount) > 0) {
          const double second_obj = model.get(GRB_DoubleAttr_ObjVal);

          std::vector<int> second_tour(n);
          if (extractTour(second_tour, nullptr) && sanitize_path(second_tour, i, j)) {
            result.second_tours[i][j] = std::move(second_tour);
            result.second_path_lengths[i][j] = second_obj - D[i][j];
          }
        } else {
          result.second_path_lengths[i][j] = std::numeric_limits<double>::infinity();
        }

        model.remove(ban_constr);
        model.update();
      }
    }

    // Restore last forced edge bounds (optional cleanliness)
    if (prev_i != -1) {
      x[prev_i][prev_j].set(GRB_DoubleAttr_LB, 0.0);
      x[prev_i][prev_j].set(GRB_DoubleAttr_UB, 1.0);
      model.update();
    }

  } catch (const GRBException &e) {
    std::cout << "Error number: " << e.getErrorCode() << "\n"
              << e.getMessage() << std::endl;
  } catch (...) {
    std::cout << "Error during optimization" << std::endl;
  }

  return result;
}

bool Solver::sanitize_path(std::vector<int> &v, const int &a, const int &b) {
  if (v.empty())
    return false;

  const auto is_ab = [&](const int &x) { return x == a || x == b; };
  const std::size_t n = v.size();

  // After right-rotating by k = (n - i) % n to bring v[i] to front,
  // the last element becomes the original v[(i + n - 1) % n].
  for (std::size_t i = 0; i < n; ++i) {
    if (is_ab(v[i]) && is_ab(v[(i + n - 1) % n])) {
      std::size_t k = (n - i) % n;
      if (k)
        std::rotate(v.begin(), v.end() - k, v.end());
      return true; // now v.front() and v.back() are both in {a,b}
    }
  }
  return false; // impossible: no circularly-adjacent pair from {a,b}
}
} // namespace tsp_puzzle
