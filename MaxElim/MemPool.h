#pragma once

//#include "SpinLock.h"

struct MemPool {
  static const int64_t _cPageSize = 1 << 12;
  static const int64_t _cMaxLenPages = 1 << 12;
  static const int64_t _cAlignment = 1 << 5;

private:
  //typedef SpinSync<1 << 5> TSync;
  thread_local static MemPool _instance;

  void *_heads[_cMaxLenPages];
  //TSync _syncs[_cMaxLenPages];

public:
  MemPool() {
    memset(_heads, 0, sizeof(_heads));
  }
  ~MemPool() {
    for (int64_t i = 0; i < _cMaxLenPages; i++) {
      void *cur = _heads[i];
      while (cur != nullptr) {
        void *next = *reinterpret_cast<void**>(cur);
        _mm_free(cur);
        cur = next;
      }
    }
  }

  static MemPool& Instance() { return _instance; }
  static int64_t RoundUp(const int64_t nBytes) { return ((nBytes - 1) / _cPageSize + 1) * _cPageSize; }

  void *Acquire(const int64_t nBytes) {
    if (nBytes <= 0) {
      return nullptr;
    }
    const int64_t iSize = (nBytes-1) / _cPageSize;
    if (iSize >= _cMaxLenPages) {
      return _mm_malloc((iSize + 1)*_cPageSize, _cAlignment);
    }
    
    //SyncLock<TSync> sl(_syncs[iSize]);
    void *ans = _heads[iSize];
    if (ans == nullptr) {
      //sl.EarlyRelease();
      return _mm_malloc((iSize + 1)*_cPageSize, _cAlignment);
    }
    _heads[iSize] = *reinterpret_cast<void**>(ans);
    return ans;
  }

  void Release(void *pMem, const int64_t nBytes) {
    if (pMem == nullptr) {
      return;
    }
    const int64_t iSize = (nBytes - 1) / _cPageSize;
    if (iSize >= _cMaxLenPages) {
      _mm_free(pMem);
      return;
    }

    //SyncLock<TSync> sl(_syncs[iSize]);
    *reinterpret_cast<void**>(pMem) = _heads[iSize];
    _heads[iSize] = pMem;
  }

};
