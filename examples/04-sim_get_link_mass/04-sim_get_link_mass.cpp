// This example application loads a simple robot from a URDF file and simulates
// its physics in a Sai2Simulation virtual world.

#include <Sai2Model.h>

#include <iostream>
#include <string>
#include <vector>

using namespace std;

const string world_file = "resources/world.urdf";
const string robot_file = "resources/pbot.urdf";


int main() {
	cout << "Loading URDF world model file: " << world_file << endl;

	double mass;
	Eigen::Vector3d CoM;
	Eigen::Matrix3d inertia;

	auto model = new Sai2Model::Sai2Model(robot_file, true);

	model->getLinkMass(mass, CoM, inertia, "link0");

	// get mass
	std::cout << "\n\nlink 0 id : " << model->linkId("link0") << std::endl;
	std::cout << "Mass of link : " << mass << std::endl;
	std::cout << "Center of mass : " << CoM.transpose() << std::endl;
	std::cout << "Inertia : \n" << inertia << std::endl;

	return 0;
}
