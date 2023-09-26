#ifndef SAI2COMMON_LOGGER_H
#define SAI2COMMON_LOGGER_H

#include <unistd.h>

#include <chrono>
#include <fstream>
#include <Eigen/Dense>
#include <thread>
#include <vector>
#include <memory>
#include <timer/LoopTimer.h>

namespace Sai2Common {

namespace {

// Log formatter
// TODO: allow user defined log formatter
Eigen::IOFormat logVecFmt(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ");

}

// interface class
class IEigenVector {
public:
	virtual void print(std::ostream& os) const = 0;
};

// template class to encapsulate matrix pointer
template <typename Derived>
class EigenVector : public IEigenVector {
public:
	// ctor
	EigenVector(const Eigen::MatrixBase<Derived>* data) { _data = data; }
	// data to encapsulate
	const Eigen::MatrixBase<Derived>* _data;
	// implementation of pure virtual function in interface class
	void print(std::ostream& os) const {
		if(_data->cols() == 1) {
			os << _data->transpose().format(logVecFmt);
			return;
		}
		os << _data->row(0).format(logVecFmt);
		for(int i=1 ; i<_data->rows() ; i++) {
			os << ", ";
			os << _data->row(i).format(logVecFmt);
		}
	}
};

// Logger class
/*
------
Notes:
------
- This class is meant to be a high speed signal logger with low computational
impact on the application thread. The logger writes to file on a separate
thread.
- Variables to be logged are registered before the logger starts. The variables
have to persist across the lifetime of the logger, otherwise a segfault will
occur.
- Eigen vector and matrices, double, int and bool are supported
- Time since start is automatically logged as the first column of the output
file.
- Logging frequency is specified in Hz and is 100 Hz by default.
- Output is a csv file with following format, and the variables will always be
logged with the eigen values first, followed by double, int and bool values:

time,	 Vector1_0,	 Vector1_1,  Vector2_0,  Vector2_1,  double1,  int1,  bool1
0,			5.0,		5.0,		5.0,		5.0,		2.0, 	23, 	0
0.01,		5.0,		5.0,		5.0,		5.0,		2.0, 	23, 	0
0.02,		5.0,		5.0,		5.0,		5.0,		2.0, 	23, 	0
...
*/
class Logger {
public:
	/**
	 * @brief Construct a new Logger and set the log file
	 * 
	 * @param fname 
	 */
	Logger(const std::string fname);

	// disallow default, copy and assign constructor
	Logger() = delete;
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	/**
	 * @brief add eigen variable to log
	 * 
	 * @param var the eigen variable to log
	 * @param var_name the name of the variable to log for the header
	 * @return true if the variable was added successfully, false otherwise
	 */
	template <typename Derived>
	bool addToLog(const Eigen::MatrixBase<Derived>& var,
				  const std::string& var_name = "") {
		if (_f_is_logging) {
			return false;
		}
		auto e = new EigenVector<Derived>(&var);
		_eigen_vars_to_log.push_back(dynamic_cast<IEigenVector*>(e));
		_num_eigen_vars_to_log += var.size();
		for (uint i = 0; i < var.size(); i++) {
			if (!var_name.empty()) {
				_eigen_header += var_name + "_" + std::to_string(i) + ", ";
			} else {
				_eigen_header += "var" + std::to_string(_eigen_vars_to_log.size()) +
						   "_" + std::to_string(i) + ", ";
			}
		}
		return true;
	}

	/**
	 * @brief add double variable to log
	 * 
	 * @param var the double variable to log
	 * @param var_name the name of the variable to log for the header
	 * @return true if the variable was added successfully, false otherwise
	 */
	bool addToLog(const double& var, const std::string var_name = "");

	/**
	 * @brief add int variable to log
	 * 
	 * @param var the int variable to log
	 * @param var_name the name of the variable to log for the header
	 * @return true if the variable was added successfully, false otherwise
	 */
	bool addToLog(const int& var, const std::string var_name = "");

	/**
	 * @brief add bool variable to log
	 * 
	 * @param var the bool variable to log
	 * @param var_name the name of the variable to log for the header
	 * @return true if the variable was added successfully, false otherwise
	 */
	bool addToLog(const bool& var, const std::string var_name = "");

	/**
	 * @brief start logging on a new log file. If the filename is the same as
	 * before, does nothing. If the logger was already running, stops it and
	 * restarts it with the new file name.
	 *
	 * @param logging_frequency the logging frequency in Hz, 100.0 by default
	 * @return true if the logger started successfully, false otherwise
	 */
	bool newFileStart(const std::string fname,
					  const double logging_frequency = 100.0);

	/**
	 * @brief start logging on the file given in the constructor
	 * 
	 * @param logging_frequency the logging frequency in Hz, 100.0 by default
	 * @return true if the logger started successfully, false otherwise
	 */
	bool start(const double logging_frequency = 100.0);

	/**
	 * @brief stop logging and close the log file
	 * 
	 */
	void stop();

private:
	// vector of pointers to encapsulate Eigen vector objects that are
	// registered with the logger
	std::vector<const IEigenVector*> _eigen_vars_to_log;
	uint _num_eigen_vars_to_log;

	// vector of pointers to encapsulate non Eigen vector objects to log
	std::vector<const double*> _double_vars_to_log;
	std::vector<const int*> _int_vars_to_log;
	std::vector<const bool*> _bool_vars_to_log;
	uint _num_double_vars_to_log;
	uint _num_int_vars_to_log;
	uint _num_bool_vars_to_log;

	// header string
	std::string _eigen_header;
	std::string _double_header;
	std::string _int_header;
	std::string _bool_header;

	// state
	bool _f_is_logging;

	// // start time
	// system_clock::time_point _t_start;

	// // log interval in microseconds
	// unsigned int _log_interval_us;

	// maximum allowed log time in seconds
	unsigned long long _num_bytes_per_line;
	double _max_log_time;

	// log file
	std::fstream _logfile;

	// log file name
	std::string _logname;

	// thread
	std::thread _log_thread;

	// internal timer for logging
	std::shared_ptr<LoopTimer> _timer;

	// thread function for logging. Note that we are not using mutexes here, so
	// there might be weirdness
	void logWorker();
};

}  // namespace Sai2Common

#endif	// SAI2COMMON_LOGGER_H