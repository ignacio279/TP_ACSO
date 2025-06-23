#include "thread-pool.h"
#include <atomic>  // <-- necesario para std::atomic

ThreadPool::ThreadPool(size_t numThreads)
  : wts(numThreads),
    taskSem(0),
    done(false)
{
    // los atomics ya inicializan a 0 por defecto
    for (size_t i = 0; i < wts.size(); ++i) {
        wts[i].ts = thread(&ThreadPool::worker, this, int(i));
    }
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    {
        lock_guard<mutex> lock(queueLock);
        taskQueue.push(thunk);
        tasksScheduled.fetch_add(1, memory_order_relaxed);
    }
    taskSem.signal();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(waitLock);
    waitCV.wait(lock, [this] {
        return tasksCompleted.load(memory_order_acquire)
             == tasksScheduled.load(memory_order_acquire);
    });
}

ThreadPool::~ThreadPool() {
    // Esperar a que terminen las tareas pendientes
    wait();

    // Señalar a los workers que deben salir
    {
        lock_guard<mutex> lock(queueLock);
        done = true;
    }
    for (size_t i = 0; i < wts.size(); ++i) {
        taskSem.signal();
    }

    // Unirse a todos
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

        // Ejecutar la tarea fuera de cualquier candado
        task();

        // Actualizar contador y notificar si ya acabó todo
        tasksCompleted.fetch_add(1, memory_order_acq_rel);
        {
            lock_guard<mutex> lock(waitLock);
            if (tasksCompleted.load(memory_order_acquire)
             == tasksScheduled.load(memory_order_acquire))
            {
                waitCV.notify_all();
            }
        }
    }
}
