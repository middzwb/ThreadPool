#include "ThreadPool.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>

template<typename... T>
auto sum(T... t) {
    return (t + ...);
}

int main() {
    ThreadPool tp(8);
    using namespace std;
    auto start = chrono::system_clock::now();
    for (int i = 0; i < 8; ++i) {
        tp.enqueue([]() { this_thread::sleep_for(1s);});
    }
    tp.drain();
    auto delta = chrono::system_clock::now() - start;
    cout << chrono::duration_cast<chrono::milliseconds>(delta).count() << endl;

    vector<future<int>> ret;
    ret.reserve(20);
    auto lb = []() {
        return 1;
    };
    for (int i = 0; i < 20; ++i) {
        ret.push_back(tp.enqueue([i]() ->int {
                // cout << i << endl;
                printf("%d\n", i);
                return i * 10;
                } ));
        tp.enqueue(lb);
    }

    tp.drain();
    cout << "---------------------------" << endl;
    for (int i = 0; i < 20; ++i) {
        auto r = ret[i].get();
        assert(r == i * 10);
        cout << r << endl;
    }
    auto fu = tp.enqueue([](int a, int b) -> int { return a + b; }, 123, 456);
    assert(fu.get() == 579);

}
