CXX      = g++
CPPFLAGS = -std=c++11 -Wall -pthread -g

# Ejecutables
TPTST  := tptest
TCUST  := tpcustomtest
MAIN   := main
CUSTOM := custom

all: $(TPTST) $(TCUST) $(MAIN) $(CUSTOM)

$(TPTST): thread-pool.cc Semaphore.cc tptest.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) $^ -o $@

$(TCUST): thread-pool.cc Semaphore.cc tpcustomtest.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) $^ -o $@

$(MAIN): main.cc thread-pool.cc Semaphore.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) $^ -o $@

$(CUSTOM): thread-pool.cc Semaphore.cc test_custom.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) $^ -o $@

clean:
	rm -f $(TPTST) $(TCUST) $(MAIN) $(CUSTOM)
