CXX      = g++
CPPFLAGS = -std=c++11 -Wall -pthread -g

# Nombre del binario para la suite de tests personalizada
CUSTOM_BIN = custom

tptest: thread-pool.cc Semaphore.cc tptest.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) thread-pool.cc Semaphore.cc tptest.cc -o tptest

tpcustomtest: thread-pool.cc Semaphore.cc tpcustomtest.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) thread-pool.cc Semaphore.cc tpcustomtest.cc -o tpcustomtest

main: main.cc thread-pool.cc Semaphore.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) main.cc thread-pool.cc Semaphore.cc -o main

# Regla para compilar la suite de tests custom
$(CUSTOM_BIN): thread-pool.cc Semaphore.cc test_custom.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) thread-pool.cc Semaphore.cc test_custom.cc -o $(CUSTOM_BIN)

# Alias conveniente
custom: $(CUSTOM_BIN)

clean:
	rm -f tptest tpcustomtest main $(CUSTOM_BIN)
