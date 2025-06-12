CXX = g++
CPPFLAGS = -std=c++11 -Wall -pthread -g

tptest: thread-pool.cc Semaphore.cc tptest.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) thread-pool.cc Semaphore.cc tptest.cc -o tptest

tpcustomtest: thread-pool.cc Semaphore.cc tpcustomtest.cc thread-pool.h Semaphore.h
	$(CXX) $(CPPFLAGS) thread-pool.cc Semaphore.cc tpcustomtest.cc -o tpcustomtest