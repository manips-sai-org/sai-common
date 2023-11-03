// LoopTimer.h

#ifndef SAI_LOOPTIMER_H_
#define SAI_LOOPTIMER_H_

#include <signal.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

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
	 * LoopTimer::run(). If overtine monitoring is enabled, this function will
	 * return false in case the overtime monitoring conditions are not satisfied
	 * and true otherwise. If no overtime monitoring is enabled, this will
	 * return true if the timer waited and false otherwise.
	 */
	bool waitForNextLoop();

	/** \brief Number of full loops completed since calling run. */
	unsigned long long elapsedCycles();

	/** \brief Elapsed computer time since calling initializeTimer() or run() in
	 * seconds. */
	double elapsedTime();

	/** \brief Elapsed simulation time since calling initializeTimer() or run()
	 * in seconds. */
	double elapsedSimTime();

	/**
	 * @brief enables overtime monitoring. Allows a monitoring of the overtime
	 * of the loop and makes the function waitForNextLoop return false if:
	 * 1 - the latest loop overtime is higher than max_time_ms
	 * 2 - the average loop overtime is higher than max_average_time_ms
	 * 3 - the percentage of loops with overtime is higher than
	 * percentage_allowed
	 *
	 * @param max_overtime_ms                     maximum overtime allowed for a
	 * single loop in milliseconds
	 * @param max_average_overtime_ms             maximum average overtime
	 * allowed for all loops in milliseconds
	 * @param percentage_overtime_loops_allowed   maximum percentage of loops
	 * allowed to have overtime (between 0 and 100)
	 * @param print_warning                       whether to print a warning
	 * when one of the overtime monitor conditions is triggered (will slow down
	 * the program and worsen the overtimes)
	 */
	void enableOvertimeMonitoring(
		const double max_overtime_ms, const double max_average_overtime_ms,
		const double percentage_overtime_loops_allowed,
		const bool print_warning = false);

	/** \brief Print the loop frequency and the average loop time. */
	void printInfoPostRun();

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
	static void setThreadHighPriority();

	/** \brief Set the thread to real time (FIFO). Thread cannot be preempted.
	 *  Set priority as 49 (kernel and interrupts are 50).
	 * \param MAX_SAFE_STACK maximum stack size in bytes which is guaranteed
	 * safe to access without faulting
	 */
	// static void setThreadRealTime(const int MAX_SAFE_STACK = 8*1024);

private:
	static void printWarning(const std::string& message) {
		std::cout << "WARNING. LoopTimer. " << message << std::endl;
	}

	volatile bool running_ = false;

	std::chrono::high_resolution_clock::time_point t_next_;
	std::chrono::high_resolution_clock::time_point t_curr_;
	std::chrono::high_resolution_clock::time_point t_start_;
	std::chrono::nanoseconds ns_update_interval_;

	unsigned long long update_counter_ = 0;

	unsigned long long overtime_loops_counter_ = 0;
	double average_overtime_ms_ = 0.0;

	double overtime_monitor_enabled_ = false;
	double overtime_monitor_threshold_ms_ = 0.0;
	double overtime_monitor_average_threshold_ms_ = 0.0;
	double overtime_monitor_percentage_allowed_ = 0.0;
	bool overtime_monitor_print_warning_ = false;
};

}  // namespace Sai2Common

#endif /* SAI_LOOPTIMER_H_ */
