#ifndef TIMER_HPP_
#define TIMER_HPP_

typedef void (*TimerCallback)(void*);

class Timer {
private:
	TimerCallback callback;
	void* obj;
	int delay;
	bool running;

	static void startTimer(Timer* t);
	void run();
public:
	Timer(TimerCallback callback, void* obj, int delay) :
			callback(callback), obj(obj), delay(delay), running(false) {
	}

	void start();
	void stop();
};

#endif
