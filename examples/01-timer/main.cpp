#include <iostream>
#include <string>
#include <thread>

#include "Sai2Graphics.h"
#include "Sai2Simulation.h"
#include "timer/LoopTimer.h"

using namespace std;

const string world_fname = "resources/world.urdf";
const string robot_name = "RRBot";

bool fSimulationRunning = false;

// sim
void simulation(std::shared_ptr<Sai2Simulation::Sai2Simulation> sim);

int main(int argc, char** argv) {
	cout << "Loading URDF world model file: " << world_fname << endl;

	// load simulation world
	auto sim = make_shared<Sai2Simulation::Sai2Simulation>(world_fname);

	// load graphics scene
	auto graphics = make_shared<Sai2Graphics::Sai2Graphics>(world_fname);
	graphics->setBackgroundColor(0.2, 0.2, 0.2);

	cout << endl
		 << "This example simulates a double pendulum, using a timer to set "
			"the simulation to real time after a 0.5 sec initial wait."
		 << endl
		 << endl;

	// start the simulation
	thread sim_thread(simulation, sim);

	// while window is open:
	while (graphics->isWindowOpen()) {
		// update graphics.
		graphics->updateRobotGraphics(robot_name, sim->getJointPositions(robot_name));
		graphics->renderGraphicsWorld();
	}

	// stop simulation
	fSimulationRunning = false;
	sim_thread.join();

	return 0;
}

//------------------------------------------------------------------------------
void simulation(std::shared_ptr<Sai2Simulation::Sai2Simulation> sim) {
	// create a loop timer
	LoopTimer timer(1.0/sim->timestep());
	timer.initializeTimer(0.5*1e9); // 0.5 s pause before starting loop

	fSimulationRunning = true;
	while (fSimulationRunning) {
		// wait the correct amount of time
		timer.waitForNextLoop();

		// integrate forward
		sim->integrate();
	}

	// display controller run frequency at the end
	double end_time = timer.elapsedTime();
    std::cout << "\n";
    std::cout << "Simulated time            : " << sim->time() << " seconds\n";
    std::cout << "Simulation Loop run time  : " << end_time << " seconds\n";
    std::cout << "Simulation Loop updates   : " << timer.elapsedCycles() << "\n";
    std::cout << "Simulation Loop frequency : " << timer.elapsedCycles()/end_time << "Hz\n";

}