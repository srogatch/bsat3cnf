#pragma once

struct MemPool {
  static const int64_t _cPageSize = 1 << 12;
  static const int64_t _cMaxLenPages = 1 << 12;
  static const int64_t _cAlignment = 1 << 5;

private:
  static MemPool _instance;

  void *_heads[_cMaxLenPages];
  std::mutex _sync;

public:
  MemPool() {
    memset(_heads, 0, sizeof(_heads));
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
    
    std::unique_lock<std::mutex> ml(_sync);
    void *ans = _heads[iSize];
    if (ans == nullptr) {
      ml.unlock();
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

    std::unique_lock<std::mutex> ml(_sync);
    *reinterpret_cast<void**>(pMem) = _heads[iSize];
    _heads[iSize] = pMem;
  }

};