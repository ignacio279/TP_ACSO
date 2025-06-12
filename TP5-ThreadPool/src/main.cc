#include <iostream>
#include <atomic>
#include <chrono>
#include "thread-pool.h"

int main() {
    ThreadPool pool(4);

    std::atomic<int> counter(0);

    for (int i = 0; i < 100; ++i) {
        pool.schedule([&]() {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
        });
    }

    pool.wait();

    std::cout << "Contador final = " << counter << std::endl;

    return 0;
}
