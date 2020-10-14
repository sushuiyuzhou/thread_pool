### Simple C++17 Thread Pool
#### Features:

- Flexible interface: can submit all types of callable objects, similar to creating a `std::thread`.
- Simple lifetime management: pimpl idiom to maintain a movable thread pool object.
- Can specify number of threads to run; Can specify if thread pool should wait for all tasks to complete before shutdown.
- Only depend on STL libraries - cross platform.
- Easy to plugin - just include `"pool.h"`

#### Sample usages:
```C++
#include "pool.h"

int getDoubledInt(int in) { return 2 * in; }

{
  auto tp = utils::ThreadPool();
  tp.run(); // can decide to start processing tasks at any point after creation

  std::future<int> ft = tp.submit(getDoubledInt, 1);
  // assert(1==ft.get()); // optinally wait for result

  // can submit all types of callable objects:
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
    tp.submit(std::function<int(int)>{[](int i) { return i; }}, 1).get();
  }
}
```
See detailed usages in `example.h`.

`logger.h` includes a sample async logger built with `std::filesystem` and the thread pool.