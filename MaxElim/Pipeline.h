#pragma once

template <typename T> class Pipeline {
  std::condition_variable _cvCanPop;
  std::mutex _sync;
  std::stack<T> _st;
  int64_t _nActive = 0;

public:
  void SetWorkerCount(const int64_t nWorkers) {
    _nActive = nWorkers;
  }

  void Push(const T& item)
  {
    {
      std::unique_lock<std::mutex> lock(_sync);
      _st.push(item);
    }
    _cvCanPop.notify_one();
  }

  bool Pop(T &item) {
    std::unique_lock<std::mutex> lock(_sync);
    for (;;) {
      if (!_st.empty()) {
        break;
      }
      _nActive--;
      if (_nActive <= 0) {
        lock.unlock();
        _cvCanPop.notify_all();
        return false; // Pipeline depleted
      }
      _cvCanPop.wait(lock);
      _nActive++;
    }
    item = std::move(_st.top());
    _st.pop();
    return true;
  }
};

