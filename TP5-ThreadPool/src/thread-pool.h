/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (zero-argument functions that don't return a value)
 * and schedules them in FIFO order to a fixed number of worker threads.
 */

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
  * @brief Represents one worker thread in the pool.
  */
 struct worker_t {
     thread ts;                    // the actual thread
     function<void(void)> thunk;   // task to execute
     Semaphore sem{0};             // signals "work ready"
     bool idle = true;             // true if waiting for work
 };
 
 /**
  * @brief A simple fixed‐size thread pool with FIFO scheduling.
  */
 class ThreadPool {
   public:
     ThreadPool(size_t numThreads);
     void schedule(const function<void(void)>& thunk);
     void wait();
     ~ThreadPool();
 
     // non‐copyable
     ThreadPool(const ThreadPool&) = delete;
     ThreadPool& operator=(const ThreadPool&) = delete;
 
   private:
     void worker(int id);          // worker loop
     void dispatcher();            // dispatcher thread (optional)
 
     thread dt;                     // dispatcher thread (optional)
     vector<worker_t> wts;          // worker threads
 
     queue<function<void(void)>> taskQueue; // queue for tasks
     Semaphore taskSem{0};          // semaphore to signal dispatcher that there's work
     mutex queueLock;               // protects taskQueue and worker idle status
     mutex waitLock;                // protects task counters
     condition_variable waitCV;     // for waiting until all tasks are done
 
     size_t tasksScheduled = 0;     // number of tasks scheduled
     size_t tasksCompleted = 0;     // number of tasks completed
     bool done = false;             // flag indicating shutdown
 };
 
 #endif // _thread_pool_
 