#ifndef THREAD_WRAPPER_H_DEFINED
#define THREAD_WRAPPER_H_DEFINED

class ThreadWrapper {
	std::thread the_thread;
public:
 
	ThreadWrapper(void (*callback)(void* first, ...),int first, ...):the_thread(callback,first) {
		///aqui puedes hacer detach
		the_thread.detach();
	}
	~ThreadWrapper() {
		if (the_thread.joinable())
		{
        the_thread.join();
     }
	}
};


#endif