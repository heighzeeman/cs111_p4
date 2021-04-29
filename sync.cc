
#include "thread.hh"
#include "timer.hh"

Mutex::Mutex()
: ownerId(0), locked(false), waitingThreads() {}

void
Mutex::lock()
{
    if (mine())
	throw SyncError("acquiring mutex already locked by this thread");
	
    IntrGuard ig;
    if (locked) {
		waitingThreads.push(Thread::current()->Id);
		Thread::swtch();
	} else {
		ownerId = Thread::current()->Id;
		locked = true;
	}
}

void
Mutex::unlock()
{
    if (!mine())
	throw SyncError("unlocking mutex not locked by this thread");

    IntrGuard ig;
    if (!waitingThreads.empty()) {
		Thread *next = Thread::threadFromId(waitingThreads.front());
		waitingThreads.pop();
		ownerId = next->Id;
		next->schedule();
	} else locked = false;
}

bool
Mutex::mine()
{
    IntrGuard ig;
    return locked && Thread::current()->Id == this->ownerId;
}

void
Condition::wait()
{
    if (!m_.mine()) throw SyncError("Condition::wait must be called with mutex locked");

    IntrGuard ig;
    waitingThreads.push(Thread::current()->Id);
    m_.unlock();
    Thread::swtch();
    m_.lock();
}

void
Condition::signal()
{
    if (!m_.mine()) throw SyncError("Condition::signal must be called with mutex locked");

    IntrGuard ig;
    if (!waitingThreads.empty()) {
	Thread::threadFromId(waitingThreads.front())->schedule();
	waitingThreads.pop();
    }
}

void
Condition::broadcast()
{
    if (!m_.mine())
        throw SyncError("Condition::broadcast must be called with mutex locked");
						
    IntrGuard ig;
    while (!waitingThreads.empty()) {
	Thread::threadFromId(waitingThreads.front())->schedule();
	waitingThreads.pop();
    }
}
