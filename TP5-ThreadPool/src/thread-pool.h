#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Semaphore.h"

using namespace std;

typedef struct worker {
    thread ts;
    function<void(void)> thunk;
    Semaphore sem{0};
    bool idle = true;
} worker_t;

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    void schedule(const function<void(void)>& thunk);
    void wait();
    ~ThreadPool();

private:
    void worker(int id);
    void dispatcher();

    thread dt;
    vector<worker_t> wts;
    queue<function<void(void)>> taskQueue;

    Semaphore taskSem{0};
    bool done;
    mutex queueLock;
    mutex waitLock;
    condition_variable waitCV;

    size_t tasksScheduled = 0;
    size_t tasksCompleted = 0;

    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif 
