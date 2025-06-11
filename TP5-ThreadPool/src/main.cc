#include <iostream>
#include <atomic>
#include <chrono>
#include "thread-pool.h"

int main() {
    // Crear un pool con 4 trabajadores
    ThreadPool pool(4);

    std::atomic<int> counter(0);

    // Programar 100 tareas que incrementan el contador
    for (int i = 0; i < 100; ++i) {
        pool.schedule([&]() {
            // Incrementar el contador
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simula trabajo
        });
    }

    // Esperar a que todas las tareas se completen
    pool.wait();

    // Verificar que todas las tareas fueron ejecutadas
    std::cout << "Contador final = " << counter << std::endl;

    return 0;
}
