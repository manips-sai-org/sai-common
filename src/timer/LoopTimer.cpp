#include "LoopTimer.h"

#include <sys/resource.h>
#include <unistd.h>
#include <iomanip>
#include <algorithm>

namespace SaiCommon
{

	LoopTimer::LoopTimer(double frequency, unsigned int initial_wait_nanoseconds)
	{
		resetLoopFrequency(frequency);
		reinitializeTimer(initial_wait_nanoseconds);
	}

	void LoopTimer::resetLoopFrequency(double frequency)
	{
		ns_update_interval_ =
			std::chrono::nanoseconds(static_cast<unsigned int>(1e9 / frequency));
	}

	void LoopTimer::reinitializeTimer(unsigned int initial_wait_nanoseconds)
	{
		update_counter_ = 0;
		auto ns_initial_wait = std::chrono::nanoseconds(initial_wait_nanoseconds);
		t_curr_ = std::chrono::high_resolution_clock::now();
		t_start_ = t_curr_ + ns_initial_wait;
		t_next_ = t_start_;
		overtime_loops_counter_ = 0;
		average_overtime_ms_ = 0.0;
		running_ = true;
	}

	bool LoopTimer::waitForNextLoop()
	{
		auto wait_start_time = std::chrono::high_resolution_clock::now(); // for monitoring purposes

		update_counter_++;
		bool return_val = true;
		t_curr_ = std::chrono::high_resolution_clock::now();

		// update average loop time
		const double loop_wait_time_ms =
			std::chrono::duration<double>(t_next_ - t_curr_).count() * 1e3;
		average_wait_time_ms_ +=
			(loop_wait_time_ms - average_wait_time_ms_) / update_counter_;

		if (t_curr_ < t_next_)
		{
			t_curr_ = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(t_next_ - t_curr_);
			t_next_ += ns_update_interval_;
		}
		else
		{
			// calculate overtime
			const double t_overtime_ms = -loop_wait_time_ms;
			++overtime_loops_counter_;
			average_overtime_ms_ +=
				(t_overtime_ms - average_overtime_ms_) / overtime_loops_counter_;

			// if monitor activated and conditions satisfied, throw an error
			if (overtime_monitor_enabled_)
			{
				if (t_overtime_ms > overtime_monitor_threshold_ms_)
				{
					return_val = false;
					if (overtime_monitor_print_warning_)
					{
						std::cout
							<< "LoopTimer " << timer_name_
							<< ". Overtime over the allowed threshold "
							   "detected. Current overtime: "
							<< t_overtime_ms
							<< " ms, threshold: " << overtime_monitor_threshold_ms_
							<< " ms" << std::endl;
					}
				}
				if (average_overtime_ms_ > overtime_monitor_average_threshold_ms_)
				{
					return_val = false;
					if (overtime_monitor_print_warning_)
					{
						std::cout << "LoopTimer " << timer_name_
								  << ". Average overtime over the "
									 "allowed threshold "
									 "detected. Current average overtime: "
								  << average_overtime_ms_ << " ms, threshold: "
								  << overtime_monitor_average_threshold_ms_ << " ms"
								  << std::endl;
					}
				}
				if ((double)overtime_loops_counter_ / update_counter_ * 100.0 >
					overtime_monitor_percentage_allowed_)
				{
					return_val = false;
					if (overtime_monitor_print_warning_)
					{
						std::cout << "LoopTimer " << timer_name_
								  << ". Percentage of overtime over "
									 "the allowed "
									 "threshold detected. Current percentage: "
								  << (double)overtime_loops_counter_ /
										 update_counter_ * 100.0
								  << " %, threshold: "
								  << overtime_monitor_percentage_allowed_ << " %"
								  << std::endl;
					}
				}
			}
			else
			{
				return_val = false;
			}
			t_curr_ = std::chrono::high_resolution_clock::now();
			t_next_ = t_curr_ + ns_update_interval_;
		}

		// for monitoring purposes
		if (tag_monitoring_enabled_)
		{
			auto wait_end_time = std::chrono::high_resolution_clock::now();
			accumulated_wait_time_ms_ += std::chrono::duration<double>(wait_end_time - wait_start_time).count() * 1000.0;
		}

		return return_val;
	}

	unsigned long long LoopTimer::elapsedCycles() { return update_counter_; }

	double LoopTimer::elapsedTime()
	{
		return std::chrono::duration<double>(t_next_ - t_start_).count();
	}

	double LoopTimer::elapsedSimTime()
	{
		return elapsedCycles() *
			   std::chrono::duration<double>(ns_update_interval_).count();
	}

	void LoopTimer::enableOvertimeMonitoring(
		const double max_overtime_ms, const double max_average_overtime_ms,
		const double percentage_overtime_loops_allowed, const bool print_warning)
	{
		overtime_monitor_enabled_ = true;
		overtime_monitor_threshold_ms_ = max_overtime_ms;
		overtime_monitor_average_threshold_ms_ = max_average_overtime_ms;
		overtime_monitor_percentage_allowed_ = percentage_overtime_loops_allowed;
		overtime_monitor_print_warning_ = print_warning;
	}

	void LoopTimer::printInfoPostRun()
	{
		std::cout << "---------- LoopTimer statistics for " << timer_name_
				  << ": ----------\n";
		std::cout << "Elapsed time                         : " << elapsedTime()
				  << " s\n";
		std::cout << "Time that should have been elapsed   : " << elapsedSimTime()
				  << " s\n";
		std::cout << "Elapsed timer cycles                 : " << elapsedCycles()
				  << "\n";
		std::cout << "Desired running frequency            : "
				  << 1e9 / ns_update_interval_.count() << " Hz\n";
		std::cout << "Actual running frequency             : "
				  << elapsedCycles() / elapsedTime() << " Hz\n";
		std::cout << "Average loop wait time               : "
				  << average_wait_time_ms_ << " ms\n";
		std::cout << "Number of overtime cycles            : "
				  << overtime_loops_counter_ << "\n";
		std::cout << "Percentage of overtime cycles        : "
				  << (double)overtime_loops_counter_ / elapsedCycles() * 100.0
				  << " %\n";
		std::cout << "Average overtime on overtime cycles  : "
				  << average_overtime_ms_ << " ms\n";
		std::cout << std::endl;
	}

	void LoopTimer::run(void (*userCallback)(void))
	{
		reinitializeTimer(ns_update_interval_.count());

		running_ = true;
		while (running_)
		{
			waitForNextLoop();
			userCallback();
		}
	}

	void LoopTimer::setThreadHighPriority()
	{
		pid_t pid = getpid();
		int priority_status = setpriority(PRIO_PROCESS, pid, -19);
		if (priority_status)
		{
			printWarning(
				"setThreadHighPriority. Failed to set priority. You may need to "
				"run as root.");
		}
	}

	void LoopTimer::enableTagMonitoring(bool enable, double print_interval_seconds)
	{
		tag_monitoring_enabled_ = enable;
		tag_monitor_print_interval_s_ = print_interval_seconds;

		if (enable)
		{
			t_last_tag_print_ = std::chrono::high_resolution_clock::now();
			tags_window_loop_count_ = 0;
			accumulated_wait_time_ms_ = 0.0;
			tags_.clear();
			tag_order_.clear();
		}
	}

	void LoopTimer::openTag(const std::string &tag_name)
	{
		if (!tag_monitoring_enabled_)
			return;

		// If it's a new tag, keep track of the insertion order
		if (tags_.find(tag_name) == tags_.end())
		{
			tag_order_.push_back(tag_name);
		}

		tags_[tag_name].start_time = std::chrono::high_resolution_clock::now();
	}

	void LoopTimer::closeTag(const std::string &tag_name)
	{
		if (!tag_monitoring_enabled_)
			return;

		auto t_end = std::chrono::high_resolution_clock::now();
		auto it = tags_.find(tag_name);

		if (it != tags_.end())
		{
			double elapsed_ms = std::chrono::duration<double>(t_end - it->second.start_time).count() * 1000.0;
			it->second.accumulated_time_ms += elapsed_ms;
			it->second.call_count++;
		}
	}

	void LoopTimer::printMonitoringSummary()
	{
		if (!tag_monitoring_enabled_)
			return;

		tags_window_loop_count_++;

		auto t_curr = std::chrono::high_resolution_clock::now();
		double elapsed_since_print_s = std::chrono::duration<double>(t_curr - t_last_tag_print_).count();

		// Only print if the specified time interval in seconds has passed
		if (elapsed_since_print_s >= tag_monitor_print_interval_s_)
		{
			double elapsed_since_print_ms = elapsed_since_print_s * 1000.0;
			double avg_loop_time_ms = elapsed_since_print_ms / tags_window_loop_count_;

			// Dynamically find the longest tag name for proper column formatting
			int name_width = 20;
			for (const auto &name : tag_order_)
			{
				name_width = std::max(name_width, static_cast<int>(name.length()) + 2);
			}

			std::cout << "\n=== LoopTimer Profiling Summary (" << timer_name_ << ") ===\n";
			std::cout << "Window: " << std::fixed << std::setprecision(1) << elapsed_since_print_ms
					  << " ms | Loops: " << tags_window_loop_count_
					  << " | Avg Loop Step: " << std::setprecision(4) << avg_loop_time_ms << " ms\n";

			std::cout << std::left << std::setw(name_width) << "Block Name"
					  << std::right << std::setw(15) << "Avg Time (ms)"
					  << std::setw(15) << "% of Loop\n";
			std::cout << std::string(name_width + 30, '-') << "\n";

			double total_tracked_avg_ms = 0.0;

			for (const auto &name : tag_order_)
			{
				auto &tag = tags_[name];
				double avg_time_ms = tag.accumulated_time_ms / std::max(1ULL, tag.call_count);
				double percentage = (avg_time_ms / avg_loop_time_ms) * 100.0;
				total_tracked_avg_ms += avg_time_ms;

				std::cout << std::left << std::setw(name_width) << name
						  << std::right << std::setw(15) << std::fixed << std::setprecision(4) << avg_time_ms
						  << std::setw(14) << std::fixed << std::setprecision(2) << percentage << " %\n";

				tag.accumulated_time_ms = 0.0;
				tag.call_count = 0;
			}

			// Output standard wait/sleep time
			double avg_wait_ms = accumulated_wait_time_ms_ / tags_window_loop_count_;
			double wait_percentage = (avg_wait_ms / avg_loop_time_ms) * 100.0;

			std::cout << std::string(name_width + 30, '-') << "\n";
			std::cout << std::left << std::setw(name_width) << "[Wait / Sleep]"
					  << std::right << std::setw(15) << std::fixed << std::setprecision(4) << avg_wait_ms
					  << std::setw(14) << std::fixed << std::setprecision(2) << wait_percentage << " %\n";

			// Output actual untracked compute time (Code you didn't tag)
			double untracked_avg_ms = avg_loop_time_ms - total_tracked_avg_ms - avg_wait_ms;
			// Clamp to 0 just in case floating point drift makes it a tiny negative number
			untracked_avg_ms = std::max(0.0, untracked_avg_ms);
			double untracked_percentage = (untracked_avg_ms / avg_loop_time_ms) * 100.0;

			std::cout << std::left << std::setw(name_width) << "[Untracked Compute]"
					  << std::right << std::setw(15) << std::fixed << std::setprecision(4) << untracked_avg_ms
					  << std::setw(14) << std::fixed << std::setprecision(2) << untracked_percentage << " %\n";
			std::cout << std::string(name_width + 30, '=') << "\n\n";

			// Reset the window
			t_last_tag_print_ = t_curr;
			tags_window_loop_count_ = 0;
			accumulated_wait_time_ms_ = 0.0;
		}
	}
	// static void LoopTimer::setThreadRealTime(const int MAX_SAFE_STACK = 8*1024) {
	//     // Declare ourself as a real time task, priority 49.
	//     // PRREMPT_RT uses priority 50
	//     // for kernel tasklets and interrupt handler by default
	//     struct sched_param param;
	//     param.sched_priority = 49;
	//    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
	//         perror("sched_setscheduler failed");
	//         exit(-1);
	//     }

	//     // Lock memory
	//     if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
	//         perror("mlockall failed");
	//         exit(-2);
	//     }

	//     // Pre-fault our stack
	//     //int MAX_SAFE_STACK = 8*1024;
	//     unsigned char dummy[MAX_SAFE_STACK];
	//     memset(dummy, 0, MAX_SAFE_STACK);
	// }

} // namespace SaiCommon