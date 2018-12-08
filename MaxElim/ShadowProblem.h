#pragma once

#include "RawClause.h"
#include "FastVector.h"
#include "Problem.h"

struct ShadowProblem {
  FastVector<uint64_t> _cl3;
  FastVector<uint64_t> _cl2;
  FastVector<uint64_t> _vr3trees;
  FastVector<uint64_t> _vr2trees;
  FastVector<uint64_t> _avlNodes;

  const Problem *_pOrig;
  Problem *_pMod;

  int64_t CountUint64(const int64_t nBits) {
    return (nBits + 63) >> 6;
  }

  ShadowProblem(const Problem& orig, Problem &mod) {
    _pOrig = &orig;
    _pMod = &mod;
    _pMod->_pShadow = this;
    _cl3.AssignZeros(CountUint64(orig._cl3.size()));
    _cl2.AssignZeros(CountUint64(orig._cl2.size()));
    _vr3trees.AssignZeros(CountUint64(orig._vr3._trees.size()));
    _vr2trees.AssignZeros(CountUint64(orig._vr2._trees.size()));
    _avlNodes.AssignZeros(CountUint64(orig._vrc._avlNp._nodes.size()));
  }

  template<typename T> void RestoreArray(const FastVector<uint64_t>& dirty, const FastVector<T>& orig,
    FastVector<T> &mod)
  {
    for (int64_t i = 0; i < dirty.size(); i++) {
      const uint64_t cur64 = dirty[i];
      if (!cur64) {
        continue;
      }
      if (_mm_popcnt_u64(cur64) <= 16) {
        //// Note: these are byte-order dependent (little endian)
        for (int8_t i32 = 0; i32 < 2; i32++) {
          const uint32_t cur32 = reinterpret_cast<const uint32_t*>(&cur64)[i32];
          if (!cur32) {
            continue;
          }
          for (int8_t i16 = 0; i16 < 2; i16++) {
            const uint16_t cur16 = reinterpret_cast<const uint16_t*>(&cur32)[i16];
            if (!cur16) {
              continue;
            }
            if (__popcnt16(cur16) <= 4) {
              for (int8_t i8 = 0; i8 < 2; i8++) {
                const uint8_t cur8 = reinterpret_cast<const uint8_t*>(&cur16)[i8];
                if (!cur8) {
                  continue;
                }
                for (int8_t j = 0; j < 8; j++) {
                  if (!(cur8 & (1 << j))) {
                    continue;
                  }
                  const int64_t index = (i << 6) + (i32 << 5) + (i16 << 4) + (i8 << 3) + j;
                  mod.Modify(index, nullptr) = orig[index];
                }
              }
            }
            else {
              const int64_t index = (i << 6) + (i32 << 5) + (i16 << 4);
              Helper::AlignedCopy(&mod.Modify(index, nullptr), &orig[index], 16 * sizeof(T));
            }
          }
        }
      }
      else {
        const int64_t index = (i << 6);
        Helper::AlignedCopy(&mod.Modify(index, nullptr), &orig[index], 64 * sizeof(T));
      }
      
    }
    mod.SetSize(orig.size());
  }

  void Restore() {
    //// Restore arrays
    RestoreArray(_cl3, _pOrig->_cl3, _pMod->_cl3);
    RestoreArray(_cl2, _pOrig->_cl2, _pMod->_cl2);
    RestoreArray(_vr3trees, _pOrig->_vr3._trees, _pMod->_vr3._trees);
    RestoreArray(_vr2trees, _pOrig->_vr2._trees, _pMod->_vr2._trees);
    RestoreArray(_avlNodes, _pOrig->_vrc._avlNp._nodes, _pMod->_vrc._avlNp._nodes);

    //// Restore scalars
    _pMod->_vrc._avlNp._iSpare = _pOrig->_vrc._avlNp._iSpare;
    _pMod->_varVal = _pOrig->_varVal;
    _pMod->_varKnown = _pOrig->_varKnown;
    _pMod->_nKnown = _pOrig->_nKnown;
  }
};

