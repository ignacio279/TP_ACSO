/**
 * File: tpcustomtest.cc
 * ---------------------
 * Unit tests *you* write to exercise the ThreadPool in una variedad
 * de escenarios, protegiendo todas las escrituras a cout con oslock.
 */

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <functional>
#include <cstring>
#include <mutex>
#include <sys/types.h> // para contar hilos
#include <unistd.h>    // para contar hilos
#include <dirent.h>    // para opendir, readdir, closedir

#include "thread-pool.h"

using namespace std;

// mutex global para serializar todo cout
static mutex oslock;

void sleep_for(int slp){
    this_thread::sleep_for(chrono::milliseconds(slp));
}

static const size_t kNumThreads = 4;
static const size_t kNumFunctions = 10;

static void simpleTest() {
    ThreadPool pool(kNumThreads);
    for (size_t id = 0; id < kNumFunctions; id++) {
        pool.schedule([id] {
            lock_guard<mutex> lg(oslock);
            cout << "Thread (ID: " << id << ") has started." << endl;
            // lg desbloquea al salir de este scope

            size_t sleepTime = (id % 3) * 10;
            this_thread::sleep_for(chrono::milliseconds(sleepTime));

            lock_guard<mutex> lg2(oslock);
            cout << "Thread (ID: " << id << ") has finished." << endl;
        });
    }
    pool.wait();
}

static void singleThreadNoWaitTest() {
    ThreadPool pool(4);
    pool.schedule([] {
        lock_guard<mutex> lg(oslock);
        cout << "This is a test." << endl;
    });
    sleep_for(1000);
}

static void singleThreadSingleWaitTest() {
    ThreadPool pool(4);
    pool.schedule([] {
        lock_guard<mutex> lg(oslock);
        cout << "This is a test." << endl;
        this_thread::sleep_for(chrono::milliseconds(1000));
    });
    pool.wait();
}

static void noThreadsDoubleWaitTest() {
    ThreadPool pool(4);
    pool.wait();
    pool.wait();
}

static void reuseThreadPoolTest() {
    ThreadPool pool(4);
    for (size_t i = 0; i < 16; i++) {
        pool.schedule([] {
            lock_guard<mutex> lg(oslock);
            cout << "This is a test." << endl;
            this_thread::sleep_for(chrono::milliseconds(50));
        });
    }
    pool.wait();
    pool.schedule([] {
        lock_guard<mutex> lg(oslock);
        cout << "This is a code." << endl;
        this_thread::sleep_for(chrono::milliseconds(1000));
    });
    pool.wait();
}

static void buildMap(map<string, function<void(void)>>& testFunctionMap) {
    testFunctionMap["--single-thread-no-wait"]   = singleThreadNoWaitTest;
    testFunctionMap["--single-thread-single-wait"] = singleThreadSingleWaitTest;
    testFunctionMap["--no-threads-double-wait"] = noThreadsDoubleWaitTest;
    testFunctionMap["--reuse-thread-pool"]      = reuseThreadPoolTest;
    testFunctionMap["--s"]                      = simpleTest;
}

static void executeAll(const map<string, function<void(void)>>& testFunctionMap) {
    for (const auto& entry: testFunctionMap) {
        lock_guard<mutex> lg(oslock);
        cout << entry.first << ":" << endl;
        // lg desbloquea tras imprimir el flag
        entry.second();
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        lock_guard<mutex> lg(oslock);
        cout << "Ouch! I need exactly two arguments." << endl;
        return 0;
    }

    map<string, function<void(void)>> testFunctionMap;
    buildMap(testFunctionMap);
    string flag = argv[1];
    if (flag == "--all") {
        executeAll(testFunctionMap);
        return 0;
    }
    auto found = testFunctionMap.find(flag);
    if (found == testFunctionMap.end()) {
        lock_guard<mutex> lg(oslock);
        cout << "Oops... we don't recognize the flag \"" << flag << "\"." << endl;
        return 0;
    }

    found->second();
    return 0;
}
