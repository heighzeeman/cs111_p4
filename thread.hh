
#pragma once

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <map>
#include <deque>
#include <unordered_map>
#include <queue>

#include "timer.hh"


using std::size_t;

// The stack pointer holds a pointer to a stack element, where most
// stack elements are the size of a uintptr_t.
using sp_t = std::uintptr_t *;

// Handy alias for allocating bytes that will automatically be freed
// when Bytes goes out of scope.  Usage:
//     Bytes mem{new char[8192]};
using Bytes = std::unique_ptr<char[]>;


// Thread-specific implementation of Atomic Integer suitable for our threads
class AtomicInt {
public:
	int get();
	int get_modify(int delta);
	int get_add();
	int get_subtract();
	int set(int value);
	AtomicInt();
	AtomicInt(int initial);

private:
	int val;
};

class Thread {
public:

    // Create a new thread that will run a given function and will
    // have a given stack size.
    static void create(std::function<void()> main, size_t stack_size = 8192);

    // Return the currently running thread.
    static Thread *current();

    // Stop executing the current thread and switch (spelled "swtch"
    // since switch is a C++ keyword) to the next scheduled thread.
    // The current thread should already have been either re-scheduled
    // or enqueued in some wait Queue before calling this.
    static void swtch();

    // Re-schedule the current thread and switch to the next scheduled
    // thread.
    static void yield();

    // Terminate the current thread.
    [[noreturn]] static void exit();

    // Initialize preemptive threading.  If this function is called
    // once at the start of the program, it should then preempt the
    // currently running thread every usec microseconds.
    static void preempt_init(std::uint64_t usec = 100'000);

    // Schedule a thread to run when the CPU is available.  (The
    // thread should not be in a queue when you call this function.)
    void schedule();
	
	// Used to identify a thread. initial_thread has Id 0, subsequent threads increment.
	int Id;
	
	// Returns a pointer to a Thread with the given Id
	static Thread *threadFromId(int Id);
	
private:
    // Constructor that does not allocate a stack, for initial_thread only.
    Thread(std::nullptr_t);
	
	// Implementation allows default destructor
	~Thread() = default;
	
	// A Thread object for the program's initial thread.
    static Thread *initial_thread;
	
	static AtomicInt runningId;
	static AtomicInt nextId;
	static std::unordered_map<int, Thread *> threads;
	static std::queue<Thread *> readyThreads;
	static Thread *toRemove;
	static void start();
	
	// Private constructor utilised in Create function.
	Thread(std::function<void()> main, size_t size);
    
	// Fill in other fields and/or methods that you need.
	sp_t sp;
	Bytes stack;
	
	std::function<void()> toExecute;
};


// A standard mutex providing mutual exclusion.
class Mutex {
public:
    void lock();
    void unlock();

    // True if the lock is held by the current thread
    bool mine();
	Mutex();
	
private:
	int ownerId;
    bool locked;
	std::queue<int> waitingThreads;
};


// A condition variable with one small twist.  Traditionally you
// supply a mutex when you wait on a condition variable, and it is an
// error if you don't always supply the same mutex.  To avoid this
// error condition and simplify assertion checking, here you just
// supply the Mutex at the time you initialize the condition variable
// and it is implicit for all the other operations.
class Condition {
public:
    explicit Condition(Mutex &m) : m_(m) {}
    void wait();	     // Go to sleep until signaled
    void signal();	     // Signal at least one waiter if any exist
    void broadcast();	     // Signal all waiting threads

private:
    Mutex &m_;
	std::queue<int> waitingThreads;
};

// Throw this in response to incorrect use of synchronization
// primitives.  Example:
//    throw SyncError("must hold Mutex to wait on Condition");
struct SyncError : public std::logic_error {
    using std::logic_error::logic_error;
};


// An object that acquires a lock in its constructor and releases it
// in the destructor, so as to avoid the risk that you forget to
// release the lock.  An example:
//
//     if (LockGuard lg(my_mutex); true) {
//         // Do something while holding the lock
//     }
using LockGuard = std::lock_guard<Mutex>;
