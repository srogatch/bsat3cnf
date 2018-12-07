#pragma once

#include "MemPool.h"
#include "Helper.h"

template<typename T> class FastVector {
  int64_t _size;
  int64_t _capBytes;
  T *_pItems;

public:
  FastVector() {
    _size = _capBytes = 0;
    _pItems = nullptr;
  }

  FastVector(const FastVector& fellow) {
    _size = fellow._size;
    _capBytes = fellow._capBytes;
    _pItems = reinterpret_cast<T*>(MemPool::Instance().Acquire(_capBytes));
    Helper::AlignedCopy(_pItems, fellow._pItems, _size * sizeof(T));
  }
  FastVector& operator=(const FastVector& fellow) {
    if (this != &fellow) {
      MemPool::Instance().Release(_pItems, _capBytes);
      _size = fellow._size;
      _capBytes = fellow._capBytes;
      _pItems = reinterpret_cast<T*>(MemPool::Instance().Acquire(_capBytes));
      Helper::AlignedCopy(_pItems, fellow._pItems, _size * sizeof(T));
    }
    return *this;
  }

  FastVector(FastVector&& fellow) {
    _size = fellow._size;
    _capBytes = fellow._capBytes;
    _pItems = fellow._pItems;
    fellow._size = 0;
    fellow._capBytes = 0;
    fellow._pItems = nullptr;
  }
  FastVector& operator=(FastVector&& fellow) {
    if (this != &fellow) {
      MemPool::Instance().Release(_pItems, _capBytes);
      _size = fellow._size;
      _capBytes = fellow._capBytes;
      _pItems = fellow._pItems;
      fellow._size = 0;
      fellow._capBytes = 0;
      fellow._pItems = nullptr;
    }
    return *this;
  }

  ~FastVector() {
    MemPool::Instance().Release(_pItems, _capBytes);
  }

  T& operator[](const int64_t at) { return _pItems[at]; }
  const T& operator[](const int64_t at) const { return _pItems[at]; }

  int64_t size() const { return _size; }

  void emplace_back() {
    const int64_t newSizeBytes = (_size + 1) * sizeof(T);
    if (newSizeBytes > _capBytes) {
      const int64_t newCapBytes = MemPool::RoundUp((_capBytes + 1) + ((_capBytes + 1) >> 1));
      T* newItems = reinterpret_cast<T*>(MemPool::Instance().Acquire(newCapBytes));
      Helper::AlignedCopy(newItems, _pItems, _size * sizeof(T));
      MemPool::Instance().Release(_pItems, _capBytes);
      _pItems = newItems;
      _capBytes = newCapBytes;
    }
    _size++;
  }

  void pop_back() {
    _size--;
  }

  T& back() {
    return _pItems[_size - 1];
  }
};
