#pragma once

template <typename T> class Pipeline {
  struct ProbCmp {
    bool operator()(const Problem& a, const Problem& b) {
      return a._cl3.size() > b._cl3.size();
    }
  };

  std::condition_variable _cvCanPop;
  std::mutex _sync;
  std::priority_queue<T, std::vector<T>, ProbCmp> _pq;
  int64_t _nActive = 0;

public:
  void SetWorkerCount(const int64_t nWorkers) {
    _nActive = nWorkers;
  }

  void Push(const T& item)
  {
    {
      std::unique_lock<std::mutex> lock(_sync);
      _pq.push(item);
    }
    _cvCanPop.notify_one();
  }

  bool Pop(T &item) {
    std::unique_lock<std::mutex> lock(_sync);
    for (;;) {
      if (!_pq.empty()) {
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
    item = std::move(_pq.top());
    _pq.pop();
    return true;
  }
};

