//
// Created by sushuiyuzhou on 10/8/2020.
//

#ifndef TP__POOL_H
#define TP__POOL_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace utils {

class ThreadPool {

  class Queue {
    using Task = std::function<void()>;

    std::map<int, Task> _q;
    mutable std::mutex _m;
    std::condition_variable _cd;

    int _c;

  public:
    Queue() : _q{}, _m{}, _cd{}, _c{0} {}

    int submit(Task t) {
      std::lock_guard<std::mutex> lg{_m};
      _c++;
      _q.emplace(int{_c}, std::move(t));

      _cd.notify_one();

      return _c;
    }

    int prioritize(int ind) {
      std::lock_guard<std::mutex> lg{_m};

      if (_q.find(ind) == _q.end()) {
        throw std::out_of_range("Task with index" + std::to_string(ind)
                                + "doesn't exist");
      }

      int newInd = _q.begin()->first - 1;
      auto task = std::move(_q.find(ind)->second);
      _q.erase(ind);
      _q.emplace(newInd, std::move(task));

      _cd.notify_one();

      return newInd;
    }

    size_t size() const {
      std::lock_guard<std::mutex> lg{_m};
      return _q.size();
    }

    bool empty() const {
      std::lock_guard<std::mutex> lg{_m};
      return _q.empty();
    }

    std::unique_ptr<Task> try_pop() {
      std::lock_guard<std::mutex> lg{_m};

      if (!_q.empty()) {
        auto res = std::make_unique<Task>(std::move(_q.begin()->second));
        _q.erase(_q.begin());
        return res;
      }

      return {};
    }

    std::unique_ptr<Task> wait_and_pop() {
      std::unique_lock<std::mutex> lk{_m};

      if (!_cd.wait_for(lk, std::chrono::seconds(5),
                        [this]() { return !_q.empty(); })) {
        throw std::runtime_error("wait_and_pop timed out");
      }

      auto res = std::make_unique<Task>(std::move(_q.begin()->second));
      _q.erase(_q.begin());
      return res;
    }
  };

  class ThreadPoolImpl {
    Queue _q;

    std::atomic<bool> _done;
    std::atomic<bool> _hasRun;
    int _numThreads;
    std::vector<std::thread> _threads;

    bool _waitUntilTasksCompleted;

    void workerThread() {
      while (!_done || (_waitUntilTasksCompleted && !_q.empty())) {
        auto task = _q.try_pop();
        if (task) {
          task->operator()();
        }
        std::this_thread::yield();
      }
    }

  public:
    explicit ThreadPoolImpl(int numThreads = 3,
                            bool waitUntilTasksCompleted = true)
        : _done{false}, _hasRun{false}, _q{}, _numThreads{0}, _threads{},
          _waitUntilTasksCompleted(waitUntilTasksCompleted) {
      int hardwareThreads =
          static_cast<int>(std::thread::hardware_concurrency());
      _numThreads =
          numThreads < hardwareThreads ? numThreads : hardwareThreads;
    }

    ThreadPoolImpl(ThreadPoolImpl const &) = delete;
    ThreadPoolImpl &operator=(ThreadPoolImpl const &) = delete;
    ThreadPoolImpl(ThreadPoolImpl &&) = delete;
    ThreadPoolImpl &operator=(ThreadPoolImpl &&) = delete;

    ~ThreadPoolImpl() {
      _done = true;

      for (auto &t : _threads) {
        if (t.joinable()) {
          t.join();
        }
      }

      if (!_q.empty()) {
        std::cerr << "[ThreadPool]: Pending tasks discarded due to early "
                     "shutdown.\n";
      }
    }

    template<typename Func, typename... Args>
    std::future<std::result_of_t<Func(Args...)>> submit(Func &&func,
                                                        Args &&... args) {
      using resultType = std::result_of_t<Func(Args...)>;

      auto task = std::make_shared<std::packaged_task<resultType()>>(
          [_func{std::forward<Func>(func)},
           _args{std::make_tuple(std::forward<Args>(args)...)}]() {
            return std::apply(_func, _args);
          });
      auto res = task->get_future();

      _q.submit([task]() { task->operator()(); });
      return res;
    }

    void run() {
      if (_hasRun) {
        std::cerr << "[ThreadPool]: Can only run once.\n";
        return;
      } else {
        _hasRun = true;
      }

      for (int i = 0; i < _numThreads; i++) {
        _threads.emplace_back(&ThreadPoolImpl::workerThread, this);
      }
    }
  };

  std::unique_ptr<ThreadPoolImpl> _impl;

public:
  explicit ThreadPool(int numThreads = 3,
                      bool waitUntilTasksCompleted = true)
      : _impl{std::make_unique<ThreadPoolImpl>(numThreads,
                                               waitUntilTasksCompleted)} {}

  ThreadPool(ThreadPool const &) = delete;
  ThreadPool &operator=(ThreadPool const &) = delete;

  ThreadPool(ThreadPool &&other) noexcept
      : _impl{std::move(other._impl)} {}

  ThreadPool &operator=(ThreadPool &&other) noexcept {
    _impl = std::move(other._impl);
    return *this;
  }

  template<typename Func, typename... Args>
  std::future<std::result_of_t<Func(Args...)>> submit(Func &&func,
                                                      Args &&... args) {
    return _impl->submit(std::forward<Func>(func),
                         std::forward<Args>(args)...);
  }

  void run() { _impl->run(); }
};

}// namespace utils

#endif//TP__POOL_H
