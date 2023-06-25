#ifndef SAI2COMMON_LOGGER_H
#define SAI2COMMON_LOGGER_H

#include <unistd.h>

#include <chrono>
#include <fstream>
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::system_clock;
#include <Eigen/Dense>
#include <thread>
#include <vector>

namespace Logging {

// Log formatter
// TODO: allow user defined log formatter
Eigen::IOFormat logVecFmt(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ");

// interface class
class IEigenVector {
public:
	virtual void print(std::ostream& os) = 0;
};

// template class to encapsulate matrix pointer
template <typename Derived>
class EigenVector : public IEigenVector {
public:
	// ctor
	EigenVector(Eigen::MatrixBase<Derived>* data) { _data = data; }
	// data to encapsulate
	Eigen::MatrixBase<Derived>* _data;
	// implementation of pure virtual function in interface class
	void print(std::ostream& os) { os << _data->transpose().format(logVecFmt); }
};

// Logger class
/*
------------
Basic usage:
------------
```
Eigen::Vector3d my_signal;
Logger log(100000, "log.csv");
log.addVectorToLog(my_signal, "Signal");
// ^Signal_0, Signal_1 and Signal_2 are added to the header row of the log as
channel names logger.start(); // this starts the logger in a different thread

bool experiment_is_running = true; // set to false by Ctrl-C handler or UI or
redis or timer while(experiment_is_running) {
	// update my_signal
}
if(logger._f_is_logging)
	logger.stop();
```

----------------------
File rotation example:
----------------------
```
Logger log(100000); //global within application

void redisFileSetCallback(string log_file_name) {
	log.newFileStart(log_file_name); // saves the current file and starts a new
one
}

int main() {
	Eigen::Vector3d my_signal1;
	Eigen::Vector3d my_signal2;
	log.addVectorToLog(my_signal1, "Signal1");
	log.addVectorToLog(my_signal2, "Signal2");
	while(experiment_is_running) {
		// update my_signals
	}
	if(logger._f_is_logging)
		logger.stop();
	...
}
```

------
Notes:
------
- This class is meant to be a high speed signal logger with low computational
impact on the application thread. The logger writes to file on a separate
thread.
- Variables to be logged are registered before the logger starts. The variables
have to persist across the lifetime of the logger, otherwise a segfault will
occur.
- Currently, only Eigen::Vector variables are supported.
- Time since start is automatically logged as the first column of the output
file.
- Logging interval is specified in microseconds.
- Output is a csv file with following format:

timestamp,	 Signal1_0,	 Signal1_1,  Signal2_0,  Signal2_1
0,				5.0,		5.0,		5.0,		5.0
100000,			5.0,		5.0,		5.0,		5.0
200000,			5.0,		5.0,		5.0,		5.0
...
*/
class Logger {
public:
	// ctor
	Logger(long interval, std::string fname);

	// ctor without creating a log file
	Logger(long interval);

	// add Eigen vector type variable to watch
	template <typename Derived>
	bool addVectorToLog(Eigen::MatrixBase<Derived>* var,
						const std::string& var_name = "") {
		if (_f_is_logging) {
			return false;
		}
		auto e = new EigenVector<Derived>(var);
		_vars_to_log.push_back(dynamic_cast<IEigenVector*>(e));
		_num_vars_to_log += var->size();
		for (uint i = 0; i < var->size(); i++) {
			if (!var_name.empty()) {
				_header += var_name + "_" + std::to_string(i) + ", ";
			} else {
				_header += "var" + std::to_string(_vars_to_log.size()) + "_" +
						   std::to_string(i) + ", ";
			}
		}
		return true;
	}

	bool newFileStart(std::string fname);

	// start logging
	bool start();

	void stop();

private:
	// vector of pointers to encapsulated Eigen vector objects that are
	// registered with the logger
	std::vector<IEigenVector*> _vars_to_log;
	uint _num_vars_to_log;

	// header string
	std::string _header;

	// state
	bool _f_is_logging;

	// start time
	system_clock::time_point _t_start;

	// log interval in microseconds
	long _log_interval_;

	// maximum allowed log time in microseconds
	long _max_log_time_us;

	// log file
	std::fstream _logfile;

	// log file name
	std::string _logname;

	// thread
	std::thread _log_thread;

	// thread function for logging. Note that we are not using mutexes here, no
	// there might be weirdness
	void logWorker();

	// hide default constructor
	Logger() {}
};

}  // namespace Logging

#endif	// SAI2COMMON_LOGGER_H