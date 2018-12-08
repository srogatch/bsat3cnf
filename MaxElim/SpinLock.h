#pragma once

class SpinStatistics {
  template <uint32_t YP> friend class SpinSync;

private: // methods
         // Returns the total number of spins together with the current spin
  static uint64_t OnContention();

public: // methods
  static uint64_t TotalContention();
};

template<uint32_t taYieldPeriod> class SpinSync {
  std::atomic_flag _af = ATOMIC_FLAG_INIT;

public:
  void Acquire() {
    uint32_t nSpins = 0;
    while (_af.test_and_set(std::memory_order_acquire)) {
      SpinStatistics::OnContention();
      nSpins++;
      if (nSpins >= taYieldPeriod) {
        nSpins = 0;
        std::this_thread::yield();
      }
      else {
        // From https://software.intel.com/sites/landingpage/IntrinsicsGuide/ :
        // "Provide a hint to the processor that the code sequence is a spin-wait loop. This can help improve the
        //   performance and power consumption of spin-wait loops." 
        _mm_pause();
      }
    }
  }

  void Release() {
    _af.clear(std::memory_order_release);
  }
};

template<typename taSync> class SyncLock {
  taSync *_pSync;

public:
  SyncLock() : _pSync(nullptr) {
  }

  explicit SyncLock(taSync& sync) : _pSync(&sync) {
    _pSync->Acquire();
  }

  ~SyncLock() {
    if (_pSync != nullptr) {
      _pSync->Release();
    }
  }

  SyncLock(const SyncLock&) = delete;

  SyncLock& operator=(const SyncLock&) = delete;

  void Init(taSync &sync) {
    assert(_pSync == nullptr);
    sync.Acquire();
    _pSync = &sync;
  }

  void EarlyRelease() {
    _pSync->Release();
    _pSync = nullptr;
  }
};
