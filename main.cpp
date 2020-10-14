#include "example.h"

int main() {
  // threadpool accepts any callable objects with arguments
  exampleForThreadPool1();

  // threadpool is movable - can use to extend lifetime
  exampleForThreadPool2();

  // always returns a future which can be handled by user
  exampleForThreadPool3();

  // example async logger built with threadpool
  exampleLogger();

  return 0;
}
