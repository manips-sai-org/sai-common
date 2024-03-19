#include "Logger.h"

#include <iostream>

namespace Sai2Common {

namespace {
std::string getTimestamp(bool add_microseconds = false) {
	// Get the current time
	std::chrono::system_clock::time_point now =
		std::chrono::system_clock::now();
	std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

	std::ostringstream oss;
	oss << std::put_time(std::localtime(&currentTime), "%Y-%m-%d__%H:%M:%S");
	if (add_microseconds) {
		auto microseconds =
			std::chrono::duration_cast<std::chrono::microseconds>(
				now.time_since_epoch()) %
			std::chrono::seconds(1);
		oss << "." << std::setfill('0') << std::setw(6) << microseconds.count();
	}
	return oss.str();
}
}  // namespace

Logger::Logger(const std::string fname, const bool add_timestamp_to_filename)
	: _logname(fname),
	  _f_is_logging(false),
	  _max_log_time(0.0),
	  _num_eigen_vars_to_log(0),
	  _num_double_vars_to_log(0),
	  _num_int_vars_to_log(0),
	  _num_bool_vars_to_log(0),
	  _add_timestamp_to_filename(add_timestamp_to_filename) {
	_timer = std::make_shared<LoopTimer>(1000.0);
}

Logger::~Logger() {
	if (_f_is_logging) {
		stop();
	}
}

bool Logger::addToLog(const double& var, const std::string var_name) {
	if (_f_is_logging) {
		return false;
	}
	_double_vars_to_log.push_back(&var);
	_num_double_vars_to_log++;
	if (!var_name.empty()) {
		_double_header += var_name + ", ";
	} else {
		_double_header +=
			"double_var" + std::to_string(_double_vars_to_log.size()) + ", ";
	}
	return true;
}

bool Logger::addToLog(const int& var, const std::string var_name) {
	if (_f_is_logging) {
		return false;
	}
	_int_vars_to_log.push_back(&var);
	_num_int_vars_to_log++;
	if (!var_name.empty()) {
		_int_header += var_name + ", ";
	} else {
		_int_header +=
			"int_var" + std::to_string(_int_vars_to_log.size()) + ", ";
	}
	return true;
}

bool Logger::addToLog(const bool& var, const std::string var_name) {
	if (_f_is_logging) {
		return false;
	}
	_bool_vars_to_log.push_back(&var);
	_num_bool_vars_to_log++;
	if (!var_name.empty()) {
		_bool_header += var_name + ", ";
	} else {
		_bool_header +=
			"bool_var" + std::to_string(_bool_vars_to_log.size()) + ", ";
	}
	return true;
}

bool Logger::newFileStart(const std::string fname,
						  const double logging_frequency) {
	// do not overwrite old file
	if (fname.compare(_logname) == 0 && !_add_timestamp_to_filename) {
		std::cerr << "Log file name requested matches existing file. "
					 "Disregarding request."
				  << std::endl;
		return false;
	}
	if (_f_is_logging) {
		stop();
	}
	_logname = fname;
	std::string log_file_name = _add_timestamp_to_filename
									? _logname + "__" + getTimestamp()
									: _logname;
	log_file_name += ".csv";
	_logfile.open(log_file_name, std::ios::out);
	return start(logging_frequency);
}

// start logging
bool Logger::start(const double logging_frequency) {
	if (_f_is_logging) {
		std::cerr << "Trying to start Logger that is already running."
				  << std::endl;
		return false;
	}

	// open file
	std::string log_file_name = _add_timestamp_to_filename
									? _logname + "__" + getTimestamp()
									: _logname;
	log_file_name += ".csv";
	_logfile.open(log_file_name, std::ios::out);

	// set timer frequency
	_timer->resetLoopFrequency(logging_frequency);

	// set logging to true
	_f_is_logging = true;

	// complete header line
	_logfile << "time, timestamp, " << _eigen_header << _double_header
			 << _int_header << _bool_header << "\n";

	// calculate max log time to keep log under 2GB
	if (_num_eigen_vars_to_log > 0 || _num_double_vars_to_log > 0 ||
		_num_int_vars_to_log > 0 || _num_bool_vars_to_log > 0) {
		// assuming 1 written character is 1 Byte and each floating point number
		// will be written with around 10 characters, each int with around 7 and
		// each bool with 3 (including the space and comma). This is most likely
		// overestimated
		uint bytes_per_line =
			10 * (_num_eigen_vars_to_log + _num_double_vars_to_log) +
			7 * _num_int_vars_to_log + 3 * _num_bool_vars_to_log;
		_max_log_time = 2e9 / (logging_frequency * bytes_per_line);
	} else {
		_max_log_time = 3600.0;	 // 1 hour
	}

	// start logging thread by move assignment
	_log_thread = std::thread{&Logger::logWorker, this};

	return true;
}

void Logger::stop() {
	if (!_f_is_logging) {
		return;
	}
	// set logging false
	_f_is_logging = false;

	// join thread
	_log_thread.join();

	// stop timer
	_timer->stop();

	// close file
	_logfile.close();
}

void Logger::logWorker() {
	_timer->reinitializeTimer();
	while (_f_is_logging) {
		_timer->waitForNextLoop();
		_logfile << _timer->elapsedTime();
		_logfile << ", " << getTimestamp(true);
		for (auto iter : _eigen_vars_to_log) {
			_logfile << ", ";
			iter->print(_logfile);
		}
		for (auto iter : _double_vars_to_log) {
			_logfile << ", " << *iter;
		}
		for (auto iter : _int_vars_to_log) {
			_logfile << ", " << *iter;
		}
		for (auto iter : _bool_vars_to_log) {
			_logfile << ", " << *iter;
		}
		_logfile << "\n";

		// log stop on max time limit
		if (_timer->elapsedTime() > _max_log_time) {
			std::cerr << "Logging stopped due to time limit" << std::endl;
			_f_is_logging = false;
			_timer->stop();
			break;
		}
	}
	_logfile.flush();
}

}  // namespace Sai2Common