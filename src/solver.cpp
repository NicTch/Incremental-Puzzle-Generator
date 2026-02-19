#include "solver.h"
#include "gurobi_c++.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

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
std::vector<std::vector<std::vector<int>>>
Solver::batch_constrained_optimal_tour_mip(
    const std::vector<std::vector<double>> &D) {
  const int n = (int)D.size();
  // NOTE: implement factorial
  std::vector<std::vector<std::vector<int>>> all_tours(
      n, std::vector<std::vector<int>>(n, std::vector<int>{}));
  double *x = new double[n];
  double *y = new double[n];

  GRBEnv *env = NULL;
  GRBVar **vars = NULL;

  vars = new GRBVar *[n];
  for (int i = 0; i < n; i++)
    vars[i] = new GRBVar[n];

  try {
    GRBEnv env = GRBEnv(true);
    env.set(GRB_IntParam_OutputFlag, 0);
    env.start();

    GRBModel model = GRBModel(env);
    // env = new GRBEnv();
    // GRBModel model = GRBModel(*env);

    // Must set LazyConstraints parameter when using lazy constraints

    model.set(GRB_IntParam_LazyConstraints, 1);

    // Create binary decision variables

    for (int i = 0; i < n; i++) {
      for (int j = 0; j <= i; j++) {
        vars[i][j] =
            model.addVar(0.0, 1.0, D[i][j], GRB_BINARY,
                         "x_" + std::to_string(i) + "_" + std::to_string(j));
        vars[j][i] = vars[i][j];
      }
    }

    // Degree-2 constraints

    for (int i = 0; i < n; i++) {
      GRBLinExpr expr = 0;
      for (int j = 0; j < n; j++)
        expr += vars[i][j];
      model.addConstr(expr == 2, "deg2_" + std::to_string(i));
    }

    // Forbid edge from node back to itself

    for (int i = 0; i < n; i++)
      vars[i][i].set(GRB_DoubleAttr_UB, 0);

    // Set callback function

    subtourelim cb = subtourelim(vars, n);
    model.setCallback(&cb);

    // Forced Edge Constraint

    GRBLinExpr force_expr = vars[0][0];
    GRBConstr forced = model.addConstr(force_expr == 1, "forced_x_s_t");

    // Optimize model
    int pi = 0, pj = 0;
    for (int i = 0; i < n; ++i) {
      for (int j = i + 1; j < n; ++j) {
        model.chgCoeff(forced, vars[pi][pj], 0.0);
        model.chgCoeff(forced, vars[i][j], 1.0);
        model.optimize();
        pi = i;
        pj = j;

        // Extract solution

        std::vector<int> tour(n);
        if (model.get(GRB_IntAttr_SolCount) > 0) {
          double **sol = new double *[n];
          for (int i = 0; i < n; i++)
            sol[i] = model.get(GRB_DoubleAttr_X, vars[i], n);

          int len;

          Solver::findsubtour(n, sol, &len, tour.data());
          assert(len == n);

          // std::cout << "Tour: ";
          // for (i = 0; i < len; i++)
          //   std::cout << tour[i] << " ";
          // std::cout << std::endl;

          for (int i = 0; i < n; i++)
            delete[] sol[i];
          delete[] sol;
        }
        if (sanitize_path(tour, i, j)) {
          all_tours[i][j] = std::move(tour);
        } else {
          std::cout << "Could not sanitize tour between :" << i << j << "\n";
        }
      }
    }

  } catch (GRBException e) {
    std::cout << "Error number: " << e.getErrorCode() << std::endl;
    std::cout << e.getMessage() << std::endl;
  } catch (...) {
    std::cout << "Error during optimization" << std::endl;
  }

  for (int i = 0; i < n; i++)
    delete[] vars[i];
  delete[] vars;
  delete[] x;
  delete[] y;
  delete env;
  return all_tours;
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
