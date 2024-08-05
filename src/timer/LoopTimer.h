#ifndef SAI_LOOPTIMER_H_
#define SAI_LOOPTIMER_H_

#include <signal.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace Sai2Common {

/**
 * @brief This class implements a precise timer to run a loop at a specified
 * frequency, and provide monitoring options for the loop runtimes.
 *
 * @details There are 2 ways of using the LoopTimer class:
 * 
 * 1 - Explicitly implement a while loop and call waitForNextLoop() in it
 * (ideally at the start of the loop) that will wait for the correct amount of
 * time to reach the desired frequency
 * 
 * 2 - Use the run() function, which will run a loop calling a user defined
 * callback function at the specified frequency
 */
class LoopTimer {
public:
	/**
	 * @brief Construct a new Loop Timer with the specified frequency and
	 * initial wait time before starting the first loop
	 *
	 * @param frequency The frequency in Hz
	 * @param initial_wait_nanoseconds The initial wait time in nanoseconds
	 * before starting the first loop
	 */
	LoopTimer(double frequency, unsigned int initial_wait_nanoseconds = 0);

	~LoopTimer() = default;

	// disallow default, copy and assign
	LoopTimer() = delete;
	LoopTimer(const LoopTimer&) = delete;
	LoopTimer& operator=(const LoopTimer&) = delete;

	void setTimerName(const std::string& name) { timer_name_ = name; }

	/**
	 * @brief Set the loop frequency
	 *
	 * @param frequency The frequency in Hz
	 */
	void resetLoopFrequency(double frequency);

	/**
	 * @brief Reinitialize the timer.
	 *
	 * @param initial_wait_nanoseconds The wait time in nanoseconds before the
	 * timer starts again
	 */
	void reinitializeTimer(unsigned int initial_wait_nanoseconds = 0);

	/** \brief Wait for next loop. Use in your while loop. Not needed if using
	 * LoopTimer::run(). If overtine monitoring is enabled, this function will
	 * return false in case the overtime monitoring conditions are not satisfied
	 * and true otherwise. If no overtime monitoring is enabled, this will
	 * return true if the timer waited and false otherwise.
	 */

	/**
	 * @brief Wait for next loop. Use if a while loop is explicitely implemented
	 * in order to wait the correct amount of time. Not needed if using
	 * LoopTimer::run(). If overtine monitoring is enabled, this function will
	 * return false in case the overtime monitoring conditions are not satisfied
	 * and true otherwise. If no overtime monitoring is enabled, this will
	 * return true if the timer waited and false otherwise.
	 *
	 * @return true if the timer waited, or if overtime monitoring is enabled
	 * and the overtime conditions are satisfied
	 * @return false if the timer did not wait, or if overtime monitoring is
	 * enabled and the overtime conditions are not satisfied
	 */
	bool waitForNextLoop();

	/**
	 * @brief Number of full loops completed since calling run, or number of
	 * times the waitForNextLoop function was called
	 */
	unsigned long long elapsedCycles();

	/**
	 * @brief Elapsed real time (from computer cpu clock) since the timer
	 * started
	 *
	 * @return time in seconds
	 */
	double elapsedTime();

	/**
	 * @brief Elapsed "simulation" time since the timer started. This
	 * corresponds to the theoretical time that should have been elapsed if the
	 * loop was running at the desired frequency, and is computed from the
	 * number of epapsed cycles and the frequency of the loop
	 *
	 * @return time in seconds
	 */
	double elapsedSimTime();

	/**
	 * @brief Enables overtime monitoring. Allows a monitoring of the overtime
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

	/**
	 * @brief Print a summary of the loop timer at the current time, or until
	 * the stip function was called. It will print info such as the desired
	 * running frequency, the actual one, the elapsed time and elapsed sim time,
	 * and some overtime monitoring info.
	 *
	 */
	void printInfoPostRun();

	/** \brief Run a loop that calls the user_callback(). Blocking function.
	 * \param userCallback A function to call every loop.
	 */

	/**
	 * @brief Starts a loop that calls the user_callback() function at the
	 * specified frequency. This is less tested than the other method of
	 * implementing explicitely the while loop and using the waitForNextLoop()
	 * function. This function is blocking and will only return when the stop()
	 * function is called.
	 *
	 * @param userCallback the callback function to run at the specified
	 * frequency
	 */
	void run(void (*userCallback)(void));

	/** \brief Stop the loop, started by run(). Use within callback, or from a
	 * seperate thread. */

	/**
	 * @brief Stop the loop started by the run() function. This function needs
	 * to be called from within the callback function, or from a separate thread
	 * if using the run() function.
	 *
	 */
	void stop() { running_ = false; }

	/**
	 * @brief Set the Ctrl C Handler object (untested)
	 *
	 * @param userCallback A function to call when the user presses ctrl-c.
	 */
	static void setCtrlCHandler(void (*userCallback)(int)) {
		struct sigaction sigIntHandler;
		sigIntHandler.sa_handler = userCallback;
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;
		sigaction(SIGINT, &sigIntHandler, NULL);
	}

	/**
	 * @brief Set the thread to a priority of -19. Priority range is -20
	 * (highest) to 19 (lowest).
	 *
	 */
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

	std::string timer_name_ = "LoopTimer";

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

	double average_wait_time_ms_ = 0.0;
};

}  // namespace Sai2Common

#endif /* SAI_LOOPTIMER_H_ */
