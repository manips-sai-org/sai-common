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

const string STR_KEY = Sai2Common::RedisServer::KEY_PREFIX + "sai2-common-example::str_key";
const string INT_KEY = Sai2Common::RedisServer::KEY_PREFIX + "sai2-common-example::int_key";
const string DOUBLE_KEY = Sai2Common::RedisServer::KEY_PREFIX + "sai2-common-example::double_key";
const string VECTOR_KEY = Sai2Common::RedisServer::KEY_PREFIX + "sai2-common-example::vector_key";
const string MATRIX_KEY = Sai2Common::RedisServer::KEY_PREFIX + "sai2-common-example::matrix_key";

int main(int argc, char** argv) {
	// // make robot model
	// auto robot = make_shared<Sai2Model::Sai2Model>(robot_fname, false);
	// Eigen::Vector2d robot_q(0.1, 0.5);
	// robot->setQ(robot_q);
	// robot->updateModel();

	// example data that a robot would have
	int robot_dofs = 2;
	double robot_gripper_opening = 0.1;
	Vector2d robot_q = Vector2d(0.1, 0.5);
	Matrix2d robot_M;
	robot_M << 5.0, -1.5, -1.5, 1.0;

	// set up signal handler
	signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	// make redis client
	auto redis_client = Sai2Common::RedisClient();
	redis_client.connect();

	// set some values in redis database
	redis_client.set(STR_KEY, "Hello World !");
	redis_client.setInt(INT_KEY, robot_dofs);
	redis_client.setDouble(DOUBLE_KEY, robot_gripper_opening);
	redis_client.setEigen(VECTOR_KEY, robot_q);
	redis_client.setEigen(MATRIX_KEY, robot_M);

	cout << endl;
	cout << "keys read from thread 1 before the loop: " << endl;
	std::cout << STR_KEY << ":\n" << redis_client.get(STR_KEY) << endl;
	std::cout << INT_KEY << ":\n" << redis_client.getInt(INT_KEY) << endl;
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
	redis_client.addToReceiveGroup(DOUBLE_KEY, second_thread_time);

	thread second_thread(second_program);

	Sai2Common::LoopTimer timer(0.5);

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
		cout << endl;

		if(timer.elapsedTime() > 10.0) {
			stopRunning = true;
		}
	}

	second_thread.join();

	// delete keys
	redis_client.del(STR_KEY);
	redis_client.del(INT_KEY);
	redis_client.del(DOUBLE_KEY);
	redis_client.del(VECTOR_KEY);
	redis_client.del(MATRIX_KEY);

	return 0;
}

void second_program() {

	// make second redis client connected to the same database
	auto redis_client_2 = Sai2Common::RedisClient();
	redis_client_2.connect();

	cout << endl;
	cout << "keys read from thread 2 before the loop: " << endl;
	std::cout << STR_KEY << ":\n" << redis_client_2.get(STR_KEY) << endl;
	std::cout << INT_KEY << ":\n" << redis_client_2.getInt(INT_KEY) << endl;
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

	Sai2Common::LoopTimer timer(1.0);
	
	int counter = 0;
	redis_client_2.addToSendGroup(INT_KEY, counter);
	redis_client_2.addToSendGroup(DOUBLE_KEY, timer.elapsedTime());
	redis_client_2.addToSendGroup(STR_KEY, message);

	message = "Started !";


	while(!stopRunning) {
		timer.waitForNextLoop();
		counter = timer.elapsedCycles();
		redis_client_2.sendAllFromGroup();
		redis_client_2.receiveAllFromGroup();

		cout << "robot info received from first thread:" << endl;
		cout << "robot joint angles:\n" << robot_q.transpose() << endl;
		cout << "robot mass matrix:\n" << robot_M << endl;
		cout << endl;
	}
}
