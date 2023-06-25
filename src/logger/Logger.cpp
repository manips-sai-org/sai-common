#include "Logger.h"

#include <iostream>

namespace Logging {

Logger::Logger(long interval, std::string fname)
	: _log_interval_(interval),
	  _logname(fname),
	  _f_is_logging(false),
	  _max_log_time_us(0),
	  _num_vars_to_log(0) {
	// create log file
	_logfile.open(fname, std::ios::out);
	_header = "timestamp, ";
}

Logger::Logger(long interval)
	: _log_interval_(interval),
	  _logname(""),
	  _f_is_logging(false),
	  _max_log_time_us(0),
	  _num_vars_to_log(0) {
	_header = "timestamp, ";
}

bool Logger::newFileStart(std::string fname) {
	// do not overwrite old file
	if (fname.compare(_logname) == 0) {
		std::cerr << "Log file name requested matches existing file. "
					 "Disregarding request."
				  << std::endl;
		return false;
	}
	if (_f_is_logging) {
		stop();
	}
	_logfile.open(fname, std::ios::out);
	fname = _logname;
	return start();
}

// start logging
bool Logger::start() {
	// save start time
	_t_start = system_clock::now();

	// set logging to true
	_f_is_logging = true;

	// complete header line
	_logfile << _header << "\n";

	// calculate max log time to keep log under 2GB
	if (_num_vars_to_log > 0) {
		_max_log_time_us = _log_interval_ * 2e9 / (_num_vars_to_log * 7 + 10);
	} else {
		_max_log_time_us = 3600 * 1e6;	// 1 hour
	}

	// start logging thread by move assignment
	_log_thread = std::thread{&Logger::logWorker, this};

	return true;
}

void Logger::stop() {
	// set logging false
	_f_is_logging = false;

	// join thread
	_log_thread.join();

	// close file
	_logfile.close();
}

void Logger::logWorker() {
	system_clock::time_point curr_time;
	system_clock::time_point last_time = system_clock::now();
	while (_f_is_logging) {
		usleep(_log_interval_ / 2);
		curr_time = system_clock::now();
		auto time_diff =
			std::chrono::duration_cast<microseconds>(curr_time - last_time);
		if (_log_interval_ > 0 &&
			time_diff >= microseconds(static_cast<uint>(_log_interval_))) {
			microseconds t_elapsed =
				std::chrono::duration_cast<microseconds>(curr_time - _t_start);
			_logfile << t_elapsed.count();
			for (auto iter : _vars_to_log) {
				_logfile << ", ";
				iter->print(_logfile);
			}
			_logfile << "\n";

			// log stop on max time limit
			if (t_elapsed.count() > _max_log_time_us) {
				std::cerr << "Logging stopped due to time limit" << std::endl;
				break;
			}
			last_time = curr_time;
		}
	}
	_logfile.flush();
}

}  // namespace Logging