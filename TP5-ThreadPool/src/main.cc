#include <iostream>
#include <atomic>
#include <chrono>
#include "thread-pool.h"

int main() {
    ThreadPool pool(4);

    std::atomic<int> counter(0);

    // Test original
    for (int i = 0; i < 100; ++i) {
        pool.schedule([&]() {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }

    pool.wait();

    std::cout << "Contador final = " << counter << std::endl;

    // Test adicional: tareas rápidas sin espera
    std::atomic<int> quickTasksCounter(0);
    for (int i = 0; i < 50; ++i) {
        pool.schedule([&quickTasksCounter]() {
            quickTasksCounter++;
        });
    }
    pool.wait();
    std::cout << "Tareas rápidas completadas = " << quickTasksCounter << std::endl;

    // Test adicional: Verificar sincronización con mutex
    std::mutex outputMutex;
    for (int i = 0; i < 10; ++i) {
        pool.schedule([i, &outputMutex]() {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Tarea con mutex ejecutando: ID = " << i << std::endl;
        });
    }
    pool.wait();

    // Test adicional: tareas con distinta duración
    for (int i = 0; i < 5; ++i) {
        pool.schedule([i]() {
            std::cout << "Tarea larga iniciada: ID = " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
            std::cout << "Tarea larga finalizada: ID = " << i << std::endl;
        });
    }
    pool.wait();

    // Final del programa
    std::cout << "Todos los tests han terminado." << std::endl;

    return 0;
}
