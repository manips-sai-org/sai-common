#include <iostream>
#include <string>

#include "logger/Logger.h"
#include "timer/LoopTimer.h"

using namespace std;
using namespace Eigen;

int main(int argc, char** argv) {
	cout
		<< endl
		<< "This example runs a timer at 1kHz and logs the time and different "
		   "variables at 100 Hz to a file, and then at 300 Hz to a second file."
		<< endl
		<< endl;

	// variables to log
	Vector2d vec2d_to_log;
	vec2d_to_log << 1.0, 2.0;
	Vector3d vec3d_to_log;
	vec3d_to_log << 11.0, 12.0, 13.0;
	Matrix3d mat3d_to_log;
	mat3d_to_log << -1.0, -2.0, -3.0, -4.0, -5.0, -6.0, -7.0, -8.0, -9.0;
	int int_to_log = 0;
	int int_to_log2 = 1000;
	bool bool_to_log = false;
	double double_to_log = 0.0;
	double double_to_log2 = 0.0;

	// setup logger
	Sai2Common::Logger logger("log1_at_100_Hz.csv");
	logger.addToLog(vec2d_to_log, "vec2d");
	logger.addToLog(vec3d_to_log, "vec3d");
	logger.addToLog(mat3d_to_log, "mat3d");
	logger.addToLog(int_to_log, "int");
	logger.addToLog(int_to_log2, "int2");
	logger.addToLog(bool_to_log, "bool");
	logger.addToLog(double_to_log, "double");
	logger.addToLog(double_to_log2, "double2");

	// start logger
	logger.start();

	// create a loop timer
	Sai2Common::LoopTimer timer(1000.0);

	// run for 2 seconds
	while (timer.elapsedTime() < 2.0) {
		timer.waitForNextLoop();
		vec2d_to_log += 0.001 * Vector2d::Ones();
		vec3d_to_log -= 0.001 * Vector3d::Ones();
		mat3d_to_log -= 0.001 * Matrix3d::Ones();
		int_to_log++;
		int_to_log2--;
		bool_to_log = !bool_to_log;
		double_to_log += 0.001;
		double_to_log2 -= 0.001;
	}

	// stop logger and restart with new file name
	logger.stop();
	logger.newFileStart("log2_at_300_Hz.csv", 300.0);

	// reinitialize timer
	timer.reinitializeTimer();

	// run for 2 seconds
	while (timer.elapsedTime() < 2.0) {
		timer.waitForNextLoop();
		vec2d_to_log += 0.001 * Vector2d::Ones();
		vec3d_to_log -= 0.001 * Vector3d::Ones();
		mat3d_to_log -= 0.001 * Matrix3d::Ones();
		int_to_log++;
		int_to_log2--;
		bool_to_log = !bool_to_log;
		double_to_log += 0.001;
		double_to_log2 -= 0.001;
	}

	// stop logger
	logger.stop();

	return 0;
}