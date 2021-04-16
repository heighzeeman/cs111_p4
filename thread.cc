
#include <unistd.h>

#include "stack.hh"
#include "thread.hh"
#include "timer.hh"
#include <iostream>

using namespace std::string_literals;

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

Thread *Thread::initial_thread = new Thread(nullptr);

AtomicInt Thread::running_Id;
AtomicInt Thread::nextId;
std::unordered_map<int, Thread *> Thread::threads;
std::queue<Thread *, std::deque<Thread *>> Thread::readyThreads;
Thread *Thread::toRemove;

// Create a placeholder Thread for the program's initial thread (which
// already has a stack, so doesn't need one allocated).
Thread::Thread(std::nullptr_t) : Id(nextId.get_add()), sp(nullptr), stack(nullptr)
{
    // You have to implement this; depending on the contents of your
    // Thread structure, an empty function body may be sufficient.
	IntrGuard ig;
	threads[Id] = this;
	running_Id.set(Id);
	toRemove = nullptr;
}

void Thread::start() {
	current()->toExecute();
	exit();
}

Thread::Thread(std::function<void()> main, size_t stack_size) : stack(Bytes(new char[stack_size])), toExecute(main)
{
	this->Id = nextId.get_add();
    this->sp = stack_init(stack.get(), stack_size, Thread::start);
}

Thread::~Thread()
{
	std::string destmsg = "Invoking destructor for thread: "s + std::to_string(this->Id);
	std::cout << destmsg << std::endl;
    // You have to implement this
}

void
Thread::create(std::function<void()> main, size_t stack_size)
{
	Thread *newthr = new Thread(main, stack_size);
	IntrGuard ig;
	Thread::threads[newthr->Id] = newthr;
    newthr->schedule();
}

Thread *
Thread::current()
{
	IntrGuard ig;
    return threads.at(running_Id.get());
}

void
Thread::schedule()
{
	IntrGuard ig;
	Thread::readyThreads.push(this);
    // You have to implement this
}

void
Thread::swtch()
{
	IntrGuard ig;
    if (Thread::readyThreads.empty()) {
		std::string errormsg = "CurrThread: "s + std::to_string(Thread::current()->Id) +" Unable to switch to a running thread as there are none."s;
		std::cout << errormsg << std::endl;
		Thread::exit();
	}
	Thread *next = Thread::readyThreads.front();
	Thread::readyThreads.pop();
	Thread::running_Id.set(next->Id);
	stack_switch(&Thread::current()->sp, &next->sp);
}

void
Thread::yield()
{
	Thread::current()->schedule();
	Thread::swtch();
}

void
Thread::exit()
{
	IntrGuard ig;
	Thread *currthr = Thread::current();
	Thread::threads.erase(currthr->Id);
	if (currthr->Id == 0) {	// Exiting the initial thread
		std::cout << "Exiting initial thread." << std::endl;
		_exit(0);
	}
	if (Thread::toRemove != nullptr) {
		delete Thread::toRemove;
	}
	Thread::toRemove = currthr;
	
    // You have to implement this
	Thread::swtch();
    std::abort();  // Leave this line--control should never reach here
}

void
Thread::preempt_init(std::uint64_t usec)
{
    // You have to implement this
}
