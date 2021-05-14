# ThreadPool

simple `c++17` thread pool

Usage:

```c++
	// initialize thread pool
	ThreadPool tp(8);
	using namespace std;

    auto fu = tp.enqueue([](int a, int b) ->int { return a + b; }, 123, 456);
    assert(fu.get() == 579);

	for (int i = 0; i < 8; ++i) {
		tp.enqueue([]() { this_thread::sleep_for(1s); });
	}
	// wait for all task finished.
	tp.drain();
    // stop thread pool
    tp.stop();
```

