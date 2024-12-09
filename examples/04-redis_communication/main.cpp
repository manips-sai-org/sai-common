#include <iostream>
#include <string>
#include <thread>

#include "timer/LoopTimer.h"
#include "redis/RedisClient.h"

#include <signal.h>
bool stopRunning = false;
void sighandler(int){stopRunning = true;}

using namespace std;
using namespace Eigen;

void second_program();

const string STR_KEY = "str_key";
const string INT_KEY = "int_key";
const string BOOL_KEY = "bool_key";
const string DOUBLE_KEY = "double_key";
const string VECTOR_KEY = "vector_key";
const string MATRIX_KEY = "matrix_key";

const string prefix = "sai-common-example";

int main(int argc, char** argv) {
	// example data that a robot would have
	int robot_dofs = 2;
	double robot_gripper_opening = 0.1;
	bool safety_enabled = false;
	Vector2d robot_q = Vector2d(0.1, 0.5);
	Matrix2d robot_M;
	robot_M << 5.0, -1.5, -1.5, 1.0;

	// set up signal handler
	signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	// make redis client
	SaiCommon::RedisClient redis_client(prefix);
	redis_client.connect();

	// set some values in redis database
	redis_client.set(STR_KEY, "Hello World !");
	redis_client.setInt(INT_KEY, robot_dofs);
	redis_client.setBool(BOOL_KEY, safety_enabled);
	redis_client.setDouble(DOUBLE_KEY, robot_gripper_opening);
	redis_client.setEigen(VECTOR_KEY, robot_q);
	redis_client.setEigen(MATRIX_KEY, robot_M);

	cout << endl;
	cout << "keys read from thread 1 before the loop: " << endl;
	std::cout << STR_KEY << ":\n" << redis_client.get(STR_KEY) << endl;
	std::cout << INT_KEY << ":\n" << redis_client.getInt(INT_KEY) << endl;
	std::cout << BOOL_KEY << ":\n" << redis_client.getBool(BOOL_KEY) << endl;
	std::cout << DOUBLE_KEY << ":\n" << redis_client.getDouble(DOUBLE_KEY) << endl;
	std::cout << VECTOR_KEY << ":\n" << redis_client.getEigen(VECTOR_KEY).transpose() << endl;
	std::cout << MATRIX_KEY << ":\n" << redis_client.getEigen(MATRIX_KEY) << endl;
	cout << endl;

	// setup send and receive groups
	redis_client.addToSendGroup(VECTOR_KEY, robot_q);
	redis_client.addToSendGroup(MATRIX_KEY, robot_M);

	int second_thread_counter = 0;
	double second_thread_time = 0.0;
	redis_client.addToReceiveGroup(INT_KEY, second_thread_counter);
	redis_client.addToReceiveGroup(BOOL_KEY, safety_enabled);
	redis_client.addToReceiveGroup(DOUBLE_KEY, second_thread_time);

	thread second_thread(second_program);

	SaiCommon::LoopTimer timer(2.0, 1e6);

	while(!stopRunning) {
		timer.waitForNextLoop();

		robot_q += Eigen::Vector2d(0.1, 0.1);
		robot_M += Eigen::Matrix2d::Identity() * 0.01;

		redis_client.sendAllFromGroup();

		std::string second_thread_message = redis_client.get(STR_KEY);
		redis_client.receiveAllFromGroup();

		cout << "info received from second thread:" << endl;
		cout << second_thread_message << endl;
		cout << "second thread counter: " << second_thread_counter << endl;
		cout << "second thread time: " << second_thread_time << endl;
		cout << "safety enabled: " << safety_enabled << endl;
		cout << endl;

		if(timer.elapsedTime() > 10.0) {
			stopRunning = true;
		}
	}

	second_thread.join();

	// delete keys
	redis_client.del(STR_KEY);
	redis_client.del(INT_KEY);
	redis_client.del(BOOL_KEY);
	redis_client.del(DOUBLE_KEY);
	redis_client.del(VECTOR_KEY);
	redis_client.del(MATRIX_KEY);

	return 0;
}

void second_program() {

	// make second redis client connected to the same database
	SaiCommon::RedisClient redis_client_2(prefix);
	redis_client_2.connect();

	cout << endl;
	cout << "keys read from thread 2 before the loop: " << endl;
	std::cout << STR_KEY << ":\n" << redis_client_2.get(STR_KEY) << endl;
	std::cout << INT_KEY << ":\n" << redis_client_2.getInt(INT_KEY) << endl;
	std::cout << BOOL_KEY << ":\n" << redis_client_2.getBool(BOOL_KEY) << endl;
	std::cout << DOUBLE_KEY << ":\n" << redis_client_2.getDouble(DOUBLE_KEY) << endl;
	std::cout << VECTOR_KEY << ":\n" << redis_client_2.getEigen(VECTOR_KEY) << endl;
	std::cout << MATRIX_KEY << ":\n" << redis_client_2.getEigen(MATRIX_KEY) << endl;
	cout << endl;

	std::string message = "second thread loop not started";

	redis_client_2.setInt(INT_KEY, 0);
	redis_client_2.setDouble(DOUBLE_KEY, 0);
	redis_client_2.set(STR_KEY, message);

	Eigen::Vector2d robot_q = redis_client_2.getEigen(VECTOR_KEY);
	Eigen::MatrixXd robot_M = redis_client_2.getEigen(MATRIX_KEY);
	redis_client_2.addToReceiveGroup(VECTOR_KEY, robot_q);
	redis_client_2.addToReceiveGroup(MATRIX_KEY, robot_M);

	
	int counter = 0;
	double time = 0.0;
	bool enable_safety = false;
	redis_client_2.addToSendGroup(INT_KEY, counter);
	redis_client_2.addToSendGroup(BOOL_KEY, enable_safety);
	redis_client_2.addToSendGroup(DOUBLE_KEY, time);
	redis_client_2.addToSendGroup(STR_KEY, message);

	message = "Thread 2 started !";

	SaiCommon::LoopTimer timer(0.95, 1e6);

	while(!stopRunning) {
		timer.waitForNextLoop();
		counter = timer.elapsedCycles();
		time = timer.elapsedTime();
		if(counter > 5) {
			enable_safety = true;
		}

		redis_client_2.sendAllFromGroup();
		redis_client_2.receiveAllFromGroup();

		cout << "robot info received from first thread:" << endl;
		cout << "robot joint angles:\n" << robot_q.transpose() << endl;
		cout << "robot mass matrix:\n" << robot_M << endl;
		cout << endl;
	}
}
