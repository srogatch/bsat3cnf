#include "stdafx.h"
#include "SpinLock.h"
using namespace std;

namespace {
  atomic<uint64_t> gSpinSyncContention(0);
} // Anonymous namespace

uint64_t SpinStatistics::OnContention() {
  return 1 + gSpinSyncContention.fetch_add(1, std::memory_order_relaxed);
}

uint64_t SpinStatistics::TotalContention() {
  return gSpinSyncContention.load(std::memory_order_relaxed);
}

