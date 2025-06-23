#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>              // for size_t
#include <functional>           // for std::function
#include <thread>               // for std::thread
#include <vector>               // for std::vector
#include <queue>                // for std::queue
#include <mutex>                // for std::mutex
#include <condition_variable>   // for std::condition_variable
#include <atomic>               // for std::atomic
#include "Semaphore.h"          // for Semaphore

using namespace std;

/**
 * @brief Represents a worker in the thread pool.
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
     * Schedules the provided thunk to be executed by the pool.
     */
    void schedule(const function<void(void)>& thunk);

    /**
     * Blocks until all previously scheduled thunks have been executed.
     */
    void wait();

    /**
     * Waits for tareas pendientes y luego destruye el pool correctamente.
     */
    ~ThreadPool();

  private:
    void worker(int id);

    thread dt;                                // dispatcher thread (opcional)
    vector<worker_t> wts;                     // workers
    queue<function<void(void)>> taskQueue;   // queue de tareas

    Semaphore taskSem{0};                     // señales de tareas disponibles
    bool done = false;                        // flag de destrucción

    mutex queueLock;                          // protege taskQueue + done
    mutex waitLock;                           // sincroniza contador
    condition_variable waitCV;                // notifica a wait()

    // Contadores atómicos para evitar data races
    atomic<size_t> tasksScheduled{0};         // número de tareas scheduleadas
    atomic<size_t> tasksCompleted{0};         // número de tareas completadas

    // Prohibir copia/move
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif // _thread_pool_
