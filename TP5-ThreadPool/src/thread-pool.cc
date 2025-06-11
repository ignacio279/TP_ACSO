/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

 #include "thread-pool.h"

 // Constructor: starts N workers and a dispatcher thread
 ThreadPool::ThreadPool(size_t numThreads)
   : tasksScheduled(0),           // Inicializa tasksScheduled primero
     tasksCompleted(0),            // Inicializa tasksCompleted después
     done(false),                  // Luego inicializa done
     wts(numThreads)               // Inicializa wts último
 {
     for (size_t i = 0; i < wts.size(); ++i) {
         wts[i].ts = thread(&ThreadPool::worker, this, int(i));
     }
 }
 
 // schedule(): enqueue the thunk and signal a worker to process it
 void ThreadPool::schedule(const function<void(void)>& thunk) {
     {
         lock_guard<mutex> lock(queueLock);
         taskQueue.push(thunk);
         ++tasksScheduled;
     }
     taskSem.signal(); // use signal() instead of V()
 }
 
 // wait(): blocks until all scheduled tasks are completed
 void ThreadPool::wait() {
     unique_lock<mutex> lock(waitLock);
     waitCV.wait(lock, [this] {
         return tasksCompleted == tasksScheduled;
     });
 }
 
 // Destructor: waits for all tasks to finish, signals workers to exit, and joins all threads
 ThreadPool::~ThreadPool() {
     wait(); // Wait for all tasks to finish
     
     {
         lock_guard<mutex> lock(queueLock);
         done = true; // mark pool as done
     }
     
     // Wake up all workers to check for shutdown
     for (size_t i = 0; i < wts.size(); ++i) {
         taskSem.signal();
     }
 
     // Join all worker threads
     for (auto& w : wts) {
         if (w.ts.joinable()) {
             w.ts.join();
         }
     }
 }
 
 // worker loop: each thread waits for a task to execute
 void ThreadPool::worker(int /*id*/) {
     while (true) {
         taskSem.wait(); // use wait() instead of P()
         
         {
             lock_guard<mutex> lock(queueLock);
             if (done && taskQueue.empty()) {
                 break; // exit if pool is shutting down and queue is empty
             }
         }
 
         // Get task from queue
         function<void()> task;
         {
             lock_guard<mutex> lock(queueLock);
             if (taskQueue.empty()) {
                 continue; // if queue is empty, keep waiting
             }
             task = move(taskQueue.front());
             taskQueue.pop();
         }
 
         // Execute the task
         task();
 
         // Notify wait() that this task is completed
         {
             lock_guard<mutex> lock(waitLock);
             ++tasksCompleted;
             if (tasksCompleted == tasksScheduled) {
                 waitCV.notify_one(); // notify if all tasks are completed
             }
         }
     }
 }
 