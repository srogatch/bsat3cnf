#pragma once

#include "RawClause.h"

struct Problem {
  std::vector<Clause3> _cl3;
  std::vector<Clause2> _cl2;
  std::vector<bool> _varVal;
  std::vector<bool> _varKnown;
  int64_t _nKnown;

  static bool SignToBool(const int64_t var) {
    return var > 0;
  }
  void RemoveClause3(const int64_t at) {
    _cl3[at] = _cl3.back();
    _cl3.pop_back();
  }
  void RemoveClause2(const int64_t at) {
    _cl2[at] = _cl2.back();
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
    
    for (int64_t i = int64_t(_cl3.size())-1; i >= 0; i--) {
      for (int8_t j = 0; j < 3; j++) {
        const int64_t signedCl3 = _cl3[i]._vars[j];
        assert(signedCl3 != 0);
        const int64_t absCl3 = abs(signedCl3);
        if (absCl3 == absVar) {
          if (SignToBool(signedCl3) == SignToBool(signedVar)) {
            // evaluates to |true|
          }
          else {
            // Transform into 2-clause
            int8_t at = 0;
            _cl2.emplace_back();
            for (int8_t k = 0; k < 3; k++) {
              if (k == j) continue;
              _cl2.back()._vars[at] = _cl3[i]._vars[k];
              at++;
            }
          }
          RemoveClause3(i);
          break;
        }
      }
    }
    int64_t *pToApply = (int64_t *)_alloca(sizeof(int64_t) * (_varKnown.size() - _nKnown));
    int64_t nToApply = 0;
    for (int64_t i = int64_t(_cl2.size()) - 1; i>=0; i--) {
      for (int8_t j = 0; j < 2; j++) {
        const int64_t signedCl2 = _cl2[i]._vars[j];
        const int64_t absCl2 = abs(signedCl2);
        if (absVar == absCl2) {
          const int64_t signedOtherCl2 = _cl2[i]._vars[j ^ 1];
          RemoveClause2(i);
          if (SignToBool(signedVar) == SignToBool(signedCl2)) {
            // this clause just evaluates to true
          }
          else {
            pToApply[nToApply] = signedOtherCl2;
            nToApply++;
          }
          break;
        }
      }
    }
    for (int64_t i = 0; i < nToApply; i++) {
      if (!ApplyVar(pToApply[i])) {
        return false;
      }
    }
    return true;
  }
  template<const int8_t taNVars> void MarkSigns(const int64_t *pVars, std::vector<bool> &hasPlus, std::vector<bool> &hasMinus) {
    for (int8_t i = 0; i < taNVars; i++) {
      const int64_t curVar = pVars[i];
      if (SignToBool(curVar)) {
        hasPlus[curVar] = true;
      }
      else {
        hasMinus[-curVar] = true;
      }
    }
  }
  // Returns |false| if the problem is unsatisfiable.
  // Returns |true| if the problem may be satisfiable.
  bool EliminateSingleSigned() {
    std::vector<int64_t> toApply;
    std::vector<bool> hasPlus;
    std::vector<bool> hasMinus;
    for (;;) {
      toApply.clear();
      hasPlus.assign(_varVal.size(), false);
      hasMinus.assign(_varVal.size(), false);
      for (int64_t i = 0; i < int64_t(_cl3.size()); i++) {
        MarkSigns<3>(_cl3[i]._vars, hasPlus, hasMinus);
      }
      for (int64_t i = 0; i < int64_t(_cl2.size()); i++) {
        MarkSigns<2>(_cl2[i]._vars, hasPlus, hasMinus);
      }
      for (int64_t i = 1; i < _varVal.size(); i++) {
        if (hasPlus[i]) {
          if (!hasMinus[i]) {
            toApply.emplace_back(i);
          }
        }
        else {
          if (hasMinus[i]) {
            toApply.emplace_back(-i);
          }
        }
      }
      if (toApply.empty()) {
        break;
      }
      for (int64_t i = 0; i < toApply.size(); i++) {
        if (!ApplyVar(toApply[i])) {
          return false;
        }
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
      case 1: // 2-variable clause
        _cl2.emplace_back();
        _cl2.back()._vars[0] = _cl3[i]._vars[0];
        _cl2.back()._vars[1] = _cl3[i]._vars[1];
        RemoveClause3(i);
        break;
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

