
#include <unistd.h>

#include "stack.hh"
#include "thread.hh"
#include "timer.hh"
//#include <iostream>

using namespace std::string_literals;


// Implementation of the AtomicInt class
AtomicInt::AtomicInt(void) : val(0) {};
AtomicInt::AtomicInt(int initial) : val(initial) {};

int AtomicInt::get(void) {
	IntrGuard ig;
	return val;
}

int AtomicInt::get_modify(int delta) {
	IntrGuard ig;
	int prev = val;
	val += delta;
	return prev;
}

int AtomicInt::get_add(void) {
	return get_modify(1);
}

int AtomicInt::get_subtract(void) {
	return get_modify(-1);
}

int AtomicInt::set(int value) {
	IntrGuard ig;
	val = value;
	return val;
}

// End implementation of AtomicInt


// Instantiate static members of class Thread
AtomicInt Thread::runningId;
AtomicInt Thread::nextId;
std::unordered_map<int, Thread *> Thread::threads;
std::queue<Thread *> Thread::readyThreads;
Thread *Thread::toRemove = nullptr;
Thread *Thread::initial_thread = new Thread(nullptr);


// Begin implementation of Thread

// Create a placeholder Thread for the program's initial thread (which
// already has a stack, so doesn't need one allocated).
Thread::Thread(std::nullptr_t) : Id(nextId.get_add()), sp(nullptr), stack(nullptr)
{
	threads[Id] = this;
	runningId.set(Id);
}

// Default handler that stack_init uses to prepare a stack for use by a new thread
void Thread::start() {
	intr_enable(true);	// Re-enable interrupts after a context switch to a newly-created thread.
	current()->toExecute();
	exit();
}

Thread::Thread(std::function<void()> main, size_t stack_size)
	: Id(nextId.get_add()), stack(Bytes(new char[stack_size])), toExecute(main)
{
    this->sp = stack_init(stack.get(), stack_size, start);
}

/*Thread::~Thread()
{

}*/	// We can use default destructor

void
Thread::create(std::function<void()> main, size_t stack_size)
{
	IntrGuard ig;
	Thread *newthr = new Thread(main, stack_size);
	threads[newthr->Id] = newthr;
    newthr->schedule();
}

Thread *
Thread::current()
{
	IntrGuard ig;
    return threads.at(runningId.get());
}

void
Thread::schedule()
{
	IntrGuard ig;
	readyThreads.push(this);
}

void
Thread::swtch()
{
	IntrGuard ig;
	Thread *prev = current();
    if (readyThreads.empty()) exit();
	
	Thread *next = readyThreads.front();
	readyThreads.pop();
	runningId.set(next->Id);
	stack_switch(&prev->sp, &next->sp);
}

void
Thread::yield()
{
	IntrGuard ig;
	current()->schedule();
	swtch();
}

Thread *
Thread::threadFromId(int Id)
{
	IntrGuard ig;
	return threads.at(Id);
}


void
Thread::exit()
{
	IntrGuard ig;
	Thread *currthr = current();
	if (toRemove != nullptr) {
		threads.erase(toRemove->Id);
		delete toRemove;
	}
	
	toRemove = currthr;
	if (!readyThreads.empty()) swtch();
	else ::exit(0);
	
    std::abort();  // Leave this line--control should never reach here
}

void
Thread::preempt_init(std::uint64_t usec)
{	
	timer_init(usec, yield);
}
