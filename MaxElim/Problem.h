#pragma once

#include "RawClause.h"
#include "VarRef.h"

struct ShadowProblem;

struct Problem {
  FastVector<Clause3> _cl3;
  FastVector<Clause2> _cl2;
  std::vector<bool> _varVal;
  std::vector<bool> _varKnown;
  int64_t _nKnown;
  VarRef<3> _vr3;
  VarRef<2> _vr2;
  VarRefCommon _vrc;
  ShadowProblem *_pShadow = nullptr;

  static bool SignToBool(const int64_t var) {
    return var > 0;
  }

  void RemoveClause3(const int64_t at);
  void RemoveClause2(const int64_t at);
  bool ApplyVar(const int64_t signedVar);
  bool ActSingleSigned(const int64_t var);
  bool EliminateSingleSigned();
  bool NormalizeInput();

  FastVector<uint64_t> *AvlNodesShadow() const;
  template<int8_t taClauseSz> FastVector<uint64_t> *TreesShadow() const;
  FastVector<uint64_t> *Cl3Shadow() const;
  FastVector<uint64_t> *Cl2Shadow() const;
};

