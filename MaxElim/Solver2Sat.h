#pragma once

#include "Problem.h"

// Taken from https://www.geeksforgeeks.org/2-satisfiability-2-sat-problem/
struct Solver2Sat {
  FastVector<FastVector<int64_t>> _adj;
  FastVector<FastVector<int64_t>> _adjInv;
  std::vector<bool> _visited;
  std::vector<bool> _visitedInv;
  FastVector<int64_t> _st;
  // The strongly-connected component for this vertex.
  FastVector<int64_t> _scc;
  // The number of strongly-connected components
  int64_t _counter = 1;
  int64_t _N; // number of variables
  int64_t _M; // number of 2-cnf clauses

  // adds edges to form the original graph 
  void addEdges(const int64_t a, const int64_t b) {
    _adj.UnshadowedModify(a).emplace_back();
    _adj.UnshadowedModify(a).UnshadowedModifyBack() = b;
  }

  // add edges to form the inverse graph 
  void addEdgesInverse(const int64_t a, const int64_t b) {
    _adjInv.UnshadowedModify(b).emplace_back();
    _adjInv.UnshadowedModify(b).UnshadowedModifyBack() = a;
  }

  // for STEP 1 of Kosaraju's Algorithm 
  void dfsFirst(const int64_t u) {
    if (_visited[u])
      return;

    _visited[u] = true;

    for (int i = 0; i < _adj[u].size(); i++)
      dfsFirst(_adj[u][i]);

    _st.emplace_back();
    _st.UnshadowedModifyBack() = u;
  }

  // for STEP 2 of Kosaraju's Algorithm 
  void dfsSecond(const int64_t u) {
    if (_visitedInv[u])
      return;

    _visitedInv[u] = true;

    for (int i = 0; i < _adjInv[u].size(); i++)
      dfsSecond(_adjInv[u][i]);

    _scc.UnshadowedModify(u) = _counter;
  }

  Solver2Sat(const Problem& prob) {
    _N = prob._varVal.size() - 1;
    _M = prob._cl2.size();
    const int64_t nVertBuf = 2 * _N + 1;
    _adj.AssignZeros(nVertBuf, false);
    _adjInv.AssignZeros(nVertBuf, false);
    for (int64_t i = 0; i < nVertBuf; i++) {
      new(&_adj.UnshadowedModify(i)) FastVector<int64_t>();
      new(&_adjInv.UnshadowedModify(i)) FastVector<int64_t>();
    }
    _visited.resize(nVertBuf);
    _visitedInv.resize(nVertBuf);
    _scc.AssignZeros(nVertBuf, false);

    // adding edges to the graph 
    for (int64_t i = 0; i < _M; i++) {
      // variable x is mapped to x 
      // variable -x is mapped to n+x = n-(-x) 

      // for a[i] or b[i], addEdges -a[i] -> b[i] 
      // AND -b[i] -> a[i] 
      const int64_t a = prob._cl2[i]._vars[0];
      const int64_t b = prob._cl2[i]._vars[1];
      if (a > 0 && b > 0) {
        addEdges(a + _N, b);
        addEdgesInverse(a + _N, b);
        addEdges(b + _N, a);
        addEdgesInverse(b + _N, a);
      }
      else if (a > 0 && b < 0) {
        addEdges(a + _N, _N - b);
        addEdgesInverse(a + _N, _N - b);
        addEdges(-b, a);
        addEdgesInverse(-b, a);
      }
      else if (a < 0 && b > 0) {
        addEdges(-a, b);
        addEdgesInverse(-a, b);
        addEdges(b + _N, _N - a);
        addEdgesInverse(b + _N, _N - a);
      } 
      else {
        addEdges(-a, _N - b);
        addEdgesInverse(-a, _N - b);
        addEdges(-b, _N - a);
        addEdgesInverse(-b, _N - a);
      }
    }
  }
  ~Solver2Sat() {
    const int64_t nVertBuf = 2 * _N + 1;
    for (int64_t i = 0; i < nVertBuf; i++) {
      _adj.UnshadowedModify(i).~FastVector();
      _adjInv.UnshadowedModify(i).~FastVector();
    }
  }

  bool HasSolution() {
    // STEP 1 of Kosaraju's Algorithm which 
    // traverses the original graph 
    for (int64_t i = 1; i <= 2 * _N; i++) {
      if (!_visited[i]) {
        dfsFirst(i);
      }
    }

    // STEP 2 pf Kosaraju's Algorithm which 
    // traverses the inverse graph. After this, 
    // array scc[] stores the corresponding value 
    while (_st.size() > 0) {
      const int64_t n = _st.back();
      _st.SetSize(_st.size() - 1);

      if (!_visitedInv[n]) {
        dfsSecond(n);
        _counter++;
      }
    }

    for (int64_t i = 1; i <= _N; i++) {
      // for any 2 vairable x and -x lie in 
      // same SCC 
      if (_scc[i] == _scc[i + _N]) {
        return false;
      }
    }
    return true;
  }

  bool Solve(Problem& prob) {
    if (!HasSolution()) {
      return false;
    }
    // Taken from https://cp-algorithms.com/graph/2SAT.html
    for (int64_t i = 1; i <= _N; i++) {
      if (prob._varKnown[i]) { // unreachable known variable assignment
        continue;
      }
      prob._varKnown[i] = true;
      prob._varVal[i] = _scc[i] > _scc[i + _N];
    }
    return true;
  }
};

