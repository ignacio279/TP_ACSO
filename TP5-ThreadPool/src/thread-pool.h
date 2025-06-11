#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>           // for size_t
#include <functional>        // for std::function
#include <thread>            // for std::thread
#include <vector>            // for std::vector
#include <queue>             // for std::queue
#include <mutex>             // for std::mutex
#include <condition_variable>// for std::condition_variable
#include "Semaphore.h"       // for Semaphore

using namespace std;

/**
 * @brief Represents a worker in the thread pool.
 * 
 * The worker_t struct contains information about a worker 
 * thread in the thread pool. It includes the thread object, 
 * availability status, the task to be executed, and a semaphore 
 * (or condition variable) to signal when work is ready for the 
 * worker to process.
 */
typedef struct worker {
    thread ts;                    // the actual thread
    function<void(void)> thunk;   // task to execute
    Semaphore sem{0};             // signals "work ready"
    bool idle = true;             // true if waiting for work
} worker_t;

class ThreadPool {
  public:
    /**
     * Constructs a ThreadPool configured to spawn up to the specified
     * number of threads.
     */
    ThreadPool(size_t numThreads);

    /**
     * Schedules the provided thunk (which is something that can
     * be invoked as a zero-argument function without a return value)
     * to be executed by one of the ThreadPool's threads as soon as
     * all previously scheduled thunks have been handled.
     */
    void schedule(const function<void(void)>& thunk);

    /**
     * Blocks and waits until all previously scheduled thunks
     * have been executed in full.
     */
    void wait();

    /**
     * Waits for all previously scheduled thunks to execute, and then
     * properly brings down the ThreadPool and any resources tapped
     * over the course of its lifetime.
     */
    ~ThreadPool();

  private:
    // Worker loop, each thread waits for a task to execute
    void worker(int id);
    void dispatcher();              // Dispatcher thread (optional)
    
    thread dt;                      // dispatcher thread handle (optional)
    vector<worker_t> wts;           // worker thread handles
    queue<function<void(void)>> taskQueue; // queue for tasks

    Semaphore taskSem{0};           // semaphore to signal tasks available
    bool done;                      // flag to indicate the pool is being destroyed
    mutex queueLock;                // mutex to protect the queue of tasks
    mutex waitLock;                 // mutex for synchronizing task completion
    condition_variable waitCV;      // condition variable to notify wait()

    size_t tasksScheduled = 0;      // number of tasks scheduled
    size_t tasksCompleted = 0;      // number of tasks completed
  
    // To prevent cloning of ThreadPool objects
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif // _thread_pool_
