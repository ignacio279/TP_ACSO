CXX      = g++
CPPFLAGS = -std=c++11 -Wall -pthread -g

# Binaries
CUSTOM_BIN  = custom
TPTST_BIN   = tptest
TCUSTTST_BIN= tpcustomtest
MAIN_BIN    = main

# Reglas
$(TPTST_BIN): thread-pool.cc Semaphore.cc tptest.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) thread-pool.cc Semaphore.cc tptest.cc -o $@

$(TCUSTTST_BIN): thread-pool.cc Semaphore.cc tpcustomtest.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) thread-pool.cc Semaphore.cc tpcustomtest.cc -o $@

$(MAIN_BIN): main.cc thread-pool.cc Semaphore.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) main.cc thread-pool.cc Semaphore.cc -o $@

$(CUSTOM_BIN): thread-pool.cc Semaphore.cc test_custom.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) thread-pool.cc Semaphore.cc test_custom.cc -o $@

# Alias
custom: $(CUSTOM_BIN)

clean:
	rm -f $(TPTST_BIN) $(TCUSTTST_BIN) $(MAIN_BIN) $(CUSTOM_BIN)
