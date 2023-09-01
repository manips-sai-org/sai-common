#include <iostream>
#include <string>
#include <thread>

#include "timer/LoopTimer.h"

using namespace std;

// second thread function
void run2();

int main(int argc, char** argv) {
	cout << endl
		 << "This example runs 2 threads with one timer each at different frequencies."
		 << endl
		 << endl;

	// start the second thread
	thread thread2(run2);

	// create a loop timer
	Sai2Common::LoopTimer timer(50.0); // 50Hz timer
	timer.initializeTimer(1e6); // 1ms pause before starting loop

	// run for 1.5 seconds
	while (timer.elapsedTime() < 1.5) {
		// wait the correct amount of time
		timer.waitForNextLoop();

		std::cout << "Main thread at " << timer.elapsedTime() << " seconds." << std::endl;
	}
	timer.stop();

	// wait for second thread to finish
	thread2.join();

	// print main timer info
	std::cout << "main thread timer info:" << std::endl;
	timer.printInfoPostRun();

	return 0;
}

//------------------------------------------------------------------------------
void run2() {
	// create a loop timer
	Sai2Common::LoopTimer timer(5.0);
	timer.initializeTimer(0.5*1e9); // 0.5 s pause before starting loop

	while (timer.elapsedTime() < 3.0) {
		// wait the correct amount of time
		timer.waitForNextLoop();

		std::cout << "Second thread at " << timer.elapsedTime() << " seconds." << std::endl;
	}

	timer.printInfoPostRun();
	timer.stop();
}