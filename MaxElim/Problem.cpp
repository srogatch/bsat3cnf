#include "stdafx.h"
#include "Problem.h"
#include "ShadowProblem.h"

FastVector<uint64_t>* Problem::AvlNodesShadow() const {
  if (_pShadow == nullptr) return nullptr;
  return &_pShadow->_avlNodes;
}

template<int8_t taClauseSz> FastVector<uint64_t> *Problem::TreesShadow() const {
  if (_pShadow == nullptr) return nullptr;
  static_assert(taClauseSz == 2 || taClauseSz == 3, "We only support 2- and 3-clauses.");
  if constexpr (taClauseSz == 2) {
    return &_pShadow->_vr2trees;
  }
  else {
    return &_pShadow->_vr3trees;
  }
}

template FastVector<uint64_t> *Problem::TreesShadow<2>() const;
template FastVector<uint64_t> *Problem::TreesShadow<3>() const;

FastVector<uint64_t> *Problem::Cl3Shadow() const {
  if (_pShadow == nullptr) return nullptr;
  return &_pShadow->_cl3;
}

FastVector<uint64_t> *Problem::Cl2Shadow() const {
  if (_pShadow == nullptr) return nullptr;
  return &_pShadow->_cl2;
}

void Problem::RemoveClause3(const int64_t at) {
  const int64_t iLast = _cl3.size() - 1;
  for (int8_t j = 0; j < 3; j++) {
    _vr3.Del(_cl3[at]._vars[j], at, *this);
    if (at != iLast) {
      _vr3.Del(_cl3[iLast]._vars[j], iLast, *this);
    }
  }
  if (at != iLast) {
    _cl3.Modify(at, Cl3Shadow()) = _cl3[iLast];
    for (int8_t j = 0; j < 3; j++) {
      _vr3.Add(_cl3[at]._vars[j], at, *this);
    }
  }
  _cl3.pop_back();
}

void Problem::RemoveClause2(const int64_t at) {
  const int64_t iLast = _cl2.size() - 1;
  for (int8_t j = 0; j < 2; j++) {
    _vr2.Del(_cl2[at]._vars[j], at, *this);
    if (at != iLast) {
      _vr2.Del(_cl2[iLast]._vars[j], iLast, *this);
    }
  }
  if (at != iLast) {
    _cl2.Modify(at, Cl2Shadow()) = _cl2.back();
    for (int8_t j = 0; j < 2; j++) {
      _vr2.Add(_cl2[at]._vars[j], at, *this);
    }
  }
  _cl2.pop_back();
}
// Returns |false| if the problem is unsatisfiable.
// Returns |true| if the problem may be satisfiable.
bool Problem::ApplyVar(const int64_t signedVar) {
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

  FastVector<int64_t> toEss;
  std::vector<int64_t> clauses = _vr3.Clauses(signedVar, *this);
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
      toEss.emplace_back();
      toEss.ModifyBack(nullptr) = cl._vars[k];
    }
  }

  clauses = _vr3.Clauses(-signedVar, *this);
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
    Clause2 &cl2back = _cl2.ModifyBack(Cl2Shadow());
    for (int8_t k = 0; k < 3; k++) {
      if (k == j) continue;
      _vr2.Add(cl2back._vars[at] = _cl3[i]._vars[k], _cl2.size() - 1, *this);
      at++;
    }
    RemoveClause3(i);
  }

  clauses = _vr2.Clauses(signedVar, *this);
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
    toEss.emplace_back();
    toEss.ModifyBack(nullptr) = signedOtherCl2;
  }

  clauses = _vr2.Clauses(-signedVar, *this);
  FastVector<int64_t> toApply;
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
    toApply.emplace_back();
    toApply.ModifyBack(nullptr) = signedOtherCl2;
  }

  for (int64_t i = 0; i < toEss.size(); i++) {
    const int64_t var = toEss[i];
    if (!ActSingleSigned(var)) {
      return false;
    }
  }
  for (int64_t i = 0; i < toApply.size(); i++) {
    const int64_t var = toApply[i];
    if (!ApplyVar(var)) {
      return false;
    }
  }
  return true;
}

// Returns |false| if the problem is unsatisfiable.
// Returns |true| if the problem may be satisfiable.
bool Problem::ActSingleSigned(const int64_t var) {
  const bool straight = (_vr2.Size(var, *this) + _vr3.Size(var, *this)) > 0;
  const bool inverse = (_vr2.Size(-var, *this) + _vr3.Size(-var, *this)) > 0;
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
bool Problem::EliminateSingleSigned() {
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
bool Problem::NormalizeInput() {
  std::vector<int64_t> toApply;
  for (int64_t i = int64_t(_cl3.size()) - 1; ; i--) {
    while (i >= int64_t(_cl3.size())) {
      i--;
    }
    if (i < 0) {
      break;
    }
    int8_t j = 2;
    for (; j >= 0; j--) {
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
      Clause2 &cl2mod = _cl2.Modify(iLast, Cl2Shadow());
      int64_t var = cl2mod._vars[0] = _cl3[i]._vars[0];
      _vr2.Add(var, iLast, *this);
      var = cl2mod._vars[1] = _cl3[i]._vars[1];
      _vr2.Add(var, iLast, *this);
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