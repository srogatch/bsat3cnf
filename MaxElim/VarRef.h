#pragma once

struct VarRef {
  std::vector<std::set<int64_t>> _vrs;
  int64_t _N;

  void Init(const int64_t N) {
    _N = N;
    _vrs.resize(2 * N + 1);
  }
  void Add(const int64_t var, const int64_t iClause) {
    if (_vrs[_N + var].find(iClause) != _vrs[_N + var].end()) {
      fprintf(stderr, "Duplicate entry of variable %lld in clause %lld.\n", var, iClause);
    }
    _vrs[_N + var].emplace(iClause);
  }
  void Del(const int64_t var, const int64_t iClause) {
    _vrs[_N + var].erase(iClause);
  }
  int64_t Size(const int64_t var) {
    return _vrs[_N + var].size();
  }
  std::vector<int64_t> Clauses(const int64_t var) {
    std::vector<int64_t> ans(_vrs[_N + var].rbegin(), _vrs[_N + var].rend());
    return std::move(ans);
  }
};
