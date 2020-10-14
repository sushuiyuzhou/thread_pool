//
// Created by sushuiyuzhou on 10/14/2020.
//

#include "logger.h"
#include "pool.h"

#include <cassert>
#include <chrono>
#include <thread>

int getInt() { return 42; }

int getDoubledInt(int in) { return 2 * in; }

void doubleInt(int &in) { in *= 2; }

class IntWrapper {
  int _i;

public:
  explicit IntWrapper(int i) : _i(i) {}
  [[nodiscard]] int doubleInt() const { return 2 * _i; }
};

void exampleForThreadPool1() {
  auto tp = utils::ThreadPool();

  auto ftGetInt = tp.submit(getInt);
  auto ftGetDoubledInt = tp.submit(getDoubledInt, 1);

  tp.run();// start processing tasks, can be triggered anytime

  {
    int i{2};
    tp.submit(doubleInt, std::ref(i)).get();
    assert((i == 4) && "integer should be doubled");
  }

  {
    auto iw = IntWrapper(21);
    auto i = tp.submit(&IntWrapper::doubleInt, &iw).get();
    assert((i == 42) && "42 should be returned");
  }

  {
    tp.submit(std::function<int(int)>{[](int i) { return i; }}, 1);
  }

  assert((ftGetInt.get() == 42) && "42 should be returned");
  assert((ftGetDoubledInt.get() == 2) && "2 should be returned");

  tp.submit([] { std::cout << "all tasks finished\n"; });
}

utils::ThreadPool getTP() {
  utils::ThreadPool tp{2};

  tp.submit([] { std::cout << "task 1\n"; });
  tp.submit([] { std::this_thread::sleep_for(std::chrono::seconds(1)); });

  tp.run();

  tp.submit([] {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "task 2\n";
  });

  tp.submit([] {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "task 3\n";
  });

  return tp;
}

void exampleForThreadPool2() {
  auto tp = getTP();
  tp.run();

  auto tp2 = std::move(tp);
}

auto getFuture() {
  auto tp = utils::ThreadPool();
  tp.run();

  return tp.submit([] {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 42;
  });
}

void exampleForThreadPool3() {
  auto ft = getFuture();
  std::cout << "exampleForThreadPool3: " << ft.get();
}

void exampleLogger() {
  auto lg = utils::Logger();
  lg.info("first message");
  lg.info("second message");
  lg.warn("warning message");
  lg.error("error message");
}