// LoopTimer.h

#ifndef SAI_LOOPTIMER_H_
#define SAI_LOOPTIMER_H_

#include <signal.h>

#include <iostream>
#include <string>

#define USE_CHRONO

#ifdef USE_CHRONO
#include <chrono>
#include <thread>
#else  // USE_CHRONO

#include <time.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#endif	// USE_CHRONO

namespace Sai2Common {

/** \brief Accurately time a loop to set frequency.
 *
 */
class LoopTimer {
public:
	LoopTimer(double frequency, unsigned int initial_wait_nanoseconds = 0);

	~LoopTimer() = default;

	// disallow default, copy and assign
	LoopTimer() = delete;
	LoopTimer(const LoopTimer&) = delete;
	LoopTimer& operator=(const LoopTimer&) = delete;

	/** \brief Set the loop frequency
	 * \param frequency The loop frequency that will be used for
	 * LoopTimer::run()
	 */
	void resetLoopFrequency(double frequency);

	/** \brief Initialize the timing loop, if using your own while loop. call
	 * before waitForLoop. \param initial_wait_nanoseconds The delay before
	 * waitForNextLoop will return the first time
	 */
	void reinitializeTimer(unsigned int initial_wait_nanoseconds = 0);

	/** \brief Wait for next loop. Use in your while loop. Not needed if using
	 * LoopTimer::run(). \return true if a wait was required, and false if no
	 * wait was required. */
	bool waitForNextLoop();

	/** \brief Number of full loops completed since calling run. */
	unsigned long long elapsedCycles();

	/** \brief Time when waitForNextLoop was last called */
	double loopTime();

	/** \brief Elapsed computer time since calling initializeTimer() or run() in
	 * seconds. */
	double elapsedTime();

	/** \brief Elapsed simulation time since calling initializeTimer() or run()
	 * in seconds. */
	double elapsedSimTime();

#ifdef USE_CHRONO
	/** \brief Print the loop frequency and the average loop time. */
	void printInfoPostRun();
#endif

#ifndef USE_CHRONO
	/** \brief Time when waitForNextLoop was last called */
	void loopTime(timespec& t);

	/** \brief Elapsed time since calling initializeTimer() or run() in seconds.
	 */
	void elapsedTime(timespec& t);
#endif	// USE_CHRONO

	/** \brief Run a loop that calls the user_callback(). Blocking function.
	 * \param userCallback A function to call every loop.
	 */
	void run(void (*userCallback)(void));

	/** \brief Stop the loop, started by run(). Use within callback, or from a
	 * seperate thread. */
	void stop();

	/** \brief Add a ctr-c exit callback.
	 * \param userCallback A function to call when the user presses ctrl-c.
	 */
	static void setCtrlCHandler(void (*userCallback)(int)) {
		struct sigaction sigIntHandler;
		sigIntHandler.sa_handler = userCallback;
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;
		sigaction(SIGINT, &sigIntHandler, NULL);
	}

	/** \brief Set the thread to a priority of -19. Priority range is -20
	 * (highest) to 19 (lowest) */
	// static void setThreadHighPriority();

	/** \brief Set the thread to real time (FIFO). Thread cannot be preempted.
	 *  Set priority as 49 (kernel and interrupts are 50).
	 * \param MAX_SAFE_STACK maximum stack size in bytes which is guaranteed
	 * safe to access without faulting
	 */
	// static void setThreadRealTime(const int MAX_SAFE_STACK = 8*1024);

private:
#ifndef USE_CHRONO
	inline void getCurrentTime(timespec& t_ret);

	inline void nanoSleepUntil(const timespec& t_next, const timespec& t_now);
#endif	// !USE_CHRONO

	static void printWarning(const std::string& message) {
		std::cout << "WARNING. LoopTimer. " << message << std::endl;
	}

	volatile bool running_ = false;

#ifdef USE_CHRONO
	std::chrono::high_resolution_clock::time_point t_next_;
	std::chrono::high_resolution_clock::time_point t_curr_;
	std::chrono::high_resolution_clock::time_point t_start_;
	std::chrono::high_resolution_clock::time_point t_end_;
	std::chrono::high_resolution_clock::duration t_loop_;
	std::chrono::high_resolution_clock::duration t_tmp_;
	std::chrono::nanoseconds ns_update_interval_;
#else	// USE_CHRONO
	struct timespec t_next_;
	struct timespec t_curr_;
	struct timespec t_start_;
	struct timespec t_loop_;
	unsigned int ns_update_interval_ = 1e9 / 1000;	// 1000 Hz
#endif	// USE_CHRONO

	unsigned long long update_counter_ = 0;
};

}  // namespace Sai2Common

#endif /* SAI_LOOPTIMER_H_ */
