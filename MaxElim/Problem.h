#pragma once

#include "RawClause.h"
#include "VarRef.h"

struct Problem {
  FastVector<Clause3> _cl3;
  FastVector<Clause2> _cl2;
  std::vector<bool> _varVal;
  std::vector<bool> _varKnown;
  int64_t _nKnown;
  VarRef _vr3;
  VarRef _vr2;
  VarRefCommon _vrc;

  static bool SignToBool(const int64_t var) {
    return var > 0;
  }
  void RemoveClause3(const int64_t at) {
    const int64_t iLast = _cl3.size() - 1;
    for (int8_t j = 0; j < 3; j++) {
      _vr3.Del(_cl3[at]._vars[j], at, _vrc);
      if (at != iLast) {
        _vr3.Del(_cl3[iLast]._vars[j], iLast, _vrc);
      }
    }
    if (at != iLast) {
      _cl3[at] = _cl3[iLast];
      for (int8_t j = 0; j < 3; j++) {
        _vr3.Add(_cl3[at]._vars[j], at, _vrc);
      }
    }
    _cl3.pop_back();
  }
  void RemoveClause2(const int64_t at) {
    const int64_t iLast = _cl2.size() - 1;
    for (int8_t j = 0; j < 2; j++) {
      _vr2.Del(_cl2[at]._vars[j], at, _vrc);
      if (at != iLast) {
        _vr2.Del(_cl2[iLast]._vars[j], iLast, _vrc);
      }
    }
    if (at != iLast) {
      _cl2[at] = _cl2.back();
      for (int8_t j = 0; j < 2; j++) {
        _vr2.Add(_cl2[at]._vars[j], at, _vrc);
      }
    }
    _cl2.pop_back();
  }
  // Returns |false| if the problem is unsatisfiable.
  // Returns |true| if the problem may be satisfiable.
  bool ApplyVar(const int64_t signedVar) {
    const int64_t absVar = abs(signedVar);
    if (_varKnown[absVar]) {
      if (SignToBool(signedVar) != _varVal[absVar]) {
        return false; // unsatisfiable
      }
      return true; // nothing else to do
    }
    _varKnown[absVar] = true;
    _varVal[absVar] = SignToBool(signedVar);
    _nKnown++;
    
    std::vector<int64_t> toEss;
    std::vector<int64_t> clauses = _vr3.Clauses(signedVar, _vrc);
    for (const int64_t i : clauses) {
      int8_t j = 0;
      for (; j < 3; j++) {
        if (_cl3[i]._vars[j] == signedVar) {
          break;
        }
      }
      //if (j >= 3) {
      //  fprintf(stderr, "Inconsistency for var %lld at 3-clause %lld.\n", signedVar, i);
      //  continue;
      //}
      // evaluates to |true|
      Clause3 cl = _cl3[i];
      RemoveClause3(i);
      for (int8_t k = 0; k < 3; k++) {
        if (k == j) continue;
        toEss.emplace_back(cl._vars[k]);
      }
    }

    clauses = _vr3.Clauses(-signedVar, _vrc);
    for (const int64_t i : clauses) {
      int8_t j = 0;
      for (; j < 3; j++) {
        if (_cl3[i]._vars[j] == -signedVar) {
          break;
        }
      }
      //if (j >= 3) {
      //  fprintf(stderr, "Inconsistency for var %lld at 3-clause %lld.\n", -signedVar, i);
      //  continue;
      //}
      // Transform into 2-clause
      int8_t at = 0;
      _cl2.emplace_back();
      for (int8_t k = 0; k < 3; k++) {
        if (k == j) continue;
        _vr2.Add(_cl2.back()._vars[at] = _cl3[i]._vars[k], _cl2.size() - 1, _vrc);
        at++;
      }
      RemoveClause3(i);
    }

    std::vector<int64_t> toApply;
    clauses = _vr2.Clauses(signedVar, _vrc);
    for (const int64_t i : clauses) {
      int8_t j = 0;
      for (; j < 2; j++) {
        if (_cl2[i]._vars[j] == signedVar) {
          break;
        }
      }
      //if (j >= 2) {
      //  fprintf(stderr, "Inconsistency for var %lld at 2-clause %lld.\n", signedVar, i);
      //  continue;
      //}
      const int64_t signedOtherCl2 = _cl2[i]._vars[j ^ 1];
      RemoveClause2(i);
      // this clause just evaluates to true
      toEss.emplace_back(signedOtherCl2);
    }

    clauses = _vr2.Clauses(-signedVar, _vrc);
    for (const int64_t i : clauses) {
      int8_t j = 0;
      for (; j < 2; j++) {
        if (_cl2[i]._vars[j] == -signedVar) {
          break;
        }
      }
      //if (j >= 2) {
      //  fprintf(stderr, "Inconsistency for var %lld at 2-clause %lld.\n", -signedVar, i);
      //  continue;
      //}
      const int64_t signedOtherCl2 = _cl2[i]._vars[j ^ 1];
      RemoveClause2(i);
      toApply.emplace_back(signedOtherCl2);
    }

    for (const int64_t var : toEss) {
      if (!ActSingleSigned(var)) {
        return false;
      }
    }
    for (const int64_t var : toApply) {
      if (!ApplyVar(var)) {
        return false;
      }
    }
    return true;
  }

  // Returns |false| if the problem is unsatisfiable.
  // Returns |true| if the problem may be satisfiable.
  bool ActSingleSigned(const int64_t var) {
    const bool straight = (_vr2.Size(var, _vrc) + _vr3.Size(var, _vrc)) > 0;
    const bool inverse = (_vr2.Size(-var, _vrc) + _vr3.Size(-var, _vrc)) > 0;
    if (straight) {
      if (!inverse) {
        return ApplyVar(var);
      }
    }
    else {
      if (inverse) {
        return ApplyVar(-var);
      }
    }
    return true;
  }

  // Returns |false| if the problem is unsatisfiable.
  // Returns |true| if the problem may be satisfiable.
  bool EliminateSingleSigned() {
    for (int64_t i = 1; i < int64_t(_varVal.size()); i++) {
      if (_varKnown[i]) {
        continue;
      }
      if (!ActSingleSigned(i)) {
        return false;
      }
    }
    return true;
  }

  // Returns |false| if the problem is unsatisfiable.
  // Returns |true| if the problem may be satisfiable.
  bool NormalizeInput() {
    std::vector<int64_t> toApply;
    for (int64_t i = int64_t(_cl3.size()) - 1; ; i--) {
      while (i >= int64_t(_cl3.size())) {
        i--;
      }
      if (i < 0) {
        break;
      }
      int8_t j = 2;
      for(; j >= 0; j--) {
        if (_cl3[i]._vars[j] != 0) {
          break;
        }
      }
      switch (j) {
      case -1: // empty clause
        RemoveClause3(i);
        break;
      case 0: { // 1-variable clause
        const int64_t signedVar = _cl3[i]._vars[0];
        RemoveClause3(i);
        toApply.emplace_back(signedVar);
        break;
      }
      case 1: {// 2-variable clause
        _cl2.emplace_back();
        const int64_t iLast = _cl2.size() - 1;
        int64_t var = _cl2[iLast]._vars[0] = _cl3[i]._vars[0];
        _vr2.Add(var, iLast, _vrc);
        var = _cl2[iLast]._vars[1] = _cl3[i]._vars[1];
        _vr2.Add(var, iLast, _vrc);
        RemoveClause3(i);
        break;
      }
      default: // 3-variable clause
        break;
      }
    }
    // Now we've sorted into 1-clauses, 2-clauses and 3-clauses
    for (int64_t i = 0; i < int64_t(toApply.size()); i++) {
      if (!ApplyVar(toApply[i])) {
        return false;
      }
    }
    if (!EliminateSingleSigned()) {
      return false;
    }
    return true;
  }
};

