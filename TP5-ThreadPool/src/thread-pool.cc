#include "thread-pool.h"

ThreadPool::ThreadPool(size_t numThreads)
  : tasksScheduled(0),           // Inicializa tasksScheduled primero
    tasksCompleted(0),            // Luego tasksCompleted
    wts(numThreads),              // Luego wts
    done(false)                   // Finalmente, inicializa done
{
    for (size_t i = 0; i < wts.size(); ++i) {
        wts[i].ts = thread(&ThreadPool::worker, this, int(i)); // inicia cada worker
    }
}



// schedule(): enqueue the thunk and signal a worker to process it
void ThreadPool::schedule(const function<void(void)>& thunk) {
    {
        lock_guard<mutex> lock(queueLock); // protejo el acceso a taskQueue
        taskQueue.push(thunk);             // encolamos la tarea
        ++tasksScheduled;                  // incrementamos el contador de tareas
    }
    taskSem.signal();  // despierta a los trabajadores
}

// wait(): blocks until all scheduled tasks are completed
void ThreadPool::wait() {
    unique_lock<mutex> lock(waitLock); // protejo el contador de tareas completadas
    waitCV.wait(lock, [this] {
        return tasksCompleted == tasksScheduled; // esperamos hasta que todas las tareas se hayan completado
    });
}

// Destructor: waits for all tasks to finish, signals workers to exit, and joins all threads
ThreadPool::~ThreadPool() {
    wait(); // esperamos a que se completen todas las tareas

    {
        lock_guard<mutex> lock(queueLock); // marcamos que el pool está destruido
        done = true;
    }

    // Despertamos a todos los workers para que salgan del bucle
    for (size_t i = 0; i < wts.size(); ++i) {
        taskSem.signal();
    }

    // Unimos todos los hilos trabajadores
    for (auto& w : wts) {
        if (w.ts.joinable()) {
            w.ts.join();
        }
    }
}

// worker loop: each thread waits for a task to execute
void ThreadPool::worker(int id) {
    while (true) {
        taskSem.wait(); // espera por una tarea

        // Si el pool está terminado y la cola de tareas está vacía, el worker termina
        {
            lock_guard<mutex> lock(queueLock);
            if (done && taskQueue.empty()) {
                break;
            }
        }

        // Extraemos la tarea de la cola
        function<void()> task;
        {
            lock_guard<mutex> lock(queueLock);
            if (taskQueue.empty()) {
                continue; // si no hay tarea, seguimos esperando
            }
            task = move(taskQueue.front()); // tomamos la tarea
            taskQueue.pop(); // eliminamos la tarea de la cola
        }

        // Ejecutamos la tarea
        task();

        // Notificamos a wait() que hemos completado una tarea
        {
            lock_guard<mutex> lock(waitLock);
            ++tasksCompleted;
            if (tasksCompleted == tasksScheduled) {
                waitCV.notify_one(); // notificamos cuando todas las tareas están completas
            }
        }
    }
}
