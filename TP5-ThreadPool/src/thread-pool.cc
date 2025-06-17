#include "thread-pool.h"

ThreadPool::ThreadPool(size_t numThreads)
  : wts(numThreads),           
    taskSem(0),
    done(false),
    tasksScheduled(0),
    tasksCompleted(0)
{
    for (size_t i = 0; i < wts.size(); ++i) {
        wts[i].ts = thread(&ThreadPool::worker, this, int(i));
    }
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    {
        lock_guard<mutex> lock(queueLock); 
        taskQueue.push(thunk);             
        ++tasksScheduled;                  
    }
    taskSem.signal();  
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(waitLock); 
    waitCV.wait(lock, [this] {
        return tasksCompleted == tasksScheduled; 
    });
}

ThreadPool::~ThreadPool() {
    wait(); 

    {
        lock_guard<mutex> lock(queueLock); 
        done = true;
    }

    for (size_t i = 0; i < wts.size(); ++i) {
        taskSem.signal();
    }

    for (auto& w : wts) {
        if (w.ts.joinable()) {
            w.ts.join();
        }
    }
}

void ThreadPool::worker(int id) {
    while (true) {
        taskSem.wait(); 

        {
            lock_guard<mutex> lock(queueLock);
            if (done && taskQueue.empty()) {
                break;
            }
        }

        function<void()> task;
        {
            lock_guard<mutex> lock(queueLock);
            if (taskQueue.empty()) {
                continue; 
            }
            task = move(taskQueue.front()); 
            taskQueue.pop(); 
        }

        task();

        {
            lock_guard<mutex> lock(waitLock);
            ++tasksCompleted;
            if (tasksCompleted == tasksScheduled) {
                waitCV.notify_one(); 
            }
        }
    }
}
