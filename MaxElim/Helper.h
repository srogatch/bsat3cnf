#pragma once

#include "MemPool.h"

struct Helper {
  static void AlignedCopy(void *pDst, const void *pSrc, const int64_t nBytes) {
    const int64_t nVects = (nBytes + sizeof(__m256i) - 1) / sizeof(__m256i);
    for (int64_t i = 0; i < nVects; i++) {
      const __m256i loaded = //_mm256_stream_load_si256(reinterpret_cast<const __m256i*>(pSrc) + i);
        _mm256_load_si256(reinterpret_cast<const __m256i*>(pSrc) + i);
      //_mm256_stream_si256(reinterpret_cast<__m256i*>(pDst) + i, loaded);
      _mm256_store_si256(reinterpret_cast<__m256i*>(pDst) + i, loaded);
    }
    //_mm_sfence();
  }
};
