#include <Timer.hpp>
#include <ghost.h>
#include <ghostuser/utils/Logger.hpp>

/**
 *
 */
void Timer::start() {
	if (!running) {
		g_create_thread_d((void*) startTimer, this);
	}
}

/**
 *
 */
void Timer::stop() {
	running = false;
}

/**
 *
 */
void Timer::startTimer(Timer* t) {
	t->run();
}

/**
 *
 */
void Timer::run() {
	running = true;
	while (running) {
		g_sleep(delay);
		((TimerCallback) callback)(obj);
	}
}
