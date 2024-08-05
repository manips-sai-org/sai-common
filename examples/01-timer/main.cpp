#include <iostream>
#include <string>
#include <thread>

#include "timer/LoopTimer.h"

using namespace std;

// second thread function
void run2();

int main(int argc, char** argv) {
	cout << endl
		 << "This example runs 2 threads with one timer each at different "
			"frequencies."
		 << endl
		 << endl;

	// start the second thread
	thread thread2(run2);

	// create a loop timer
	Sai2Common::LoopTimer timer(
		50.0, 1e6);	 // 50Hz timer, 1ms pause before starting loop
	timer.setThreadHighPriority();

	// run for 1.5 seconds
	while (timer.elapsedTime() < 1.5) {
		// wait the correct amount of time
		timer.waitForNextLoop();

		std::cout << "Main thread at " << timer.elapsedTime() << " seconds."
				  << std::endl;
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
	Sai2Common::LoopTimer timer(5.0, 0.5 * 1e9);

	while (timer.elapsedTime() < 3.0) {
		// wait the correct amount of time
		timer.waitForNextLoop();

		std::cout << "Second thread at " << timer.elapsedTime() << " seconds."
				  << std::endl;
	}
	timer.stop();
	timer.printInfoPostRun();
}