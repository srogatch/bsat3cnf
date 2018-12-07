#pragma once

#include "FastVector.h"

struct AVLNode {
  int64_t _key;
  int64_t _iLeft;
  int64_t _iRight;
  int64_t _height;
};

struct AVLNodePool {
  FastVector<AVLNode> _nodes;
  int64_t _iSpare = -1;

  int64_t Acquire() {
    if (_iSpare >= 0) {
      const int64_t ans = _iSpare;
      _iSpare = _nodes[_iSpare]._iLeft;
      return ans;
    }
    const int64_t ans = _nodes.size();
    _nodes.emplace_back();
    return ans;
  }
  void Release(const int64_t iNode) {
    _nodes[iNode]._iLeft = _iSpare;
    _iSpare = iNode;
  }
};

struct AVLTree {
  int64_t _iRoot;
  int64_t _size;
};
