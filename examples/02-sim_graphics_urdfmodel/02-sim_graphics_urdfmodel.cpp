// This example application loads a URDF world file and simulates two robots
// with physics and contact in a Sai2Simulation virtual world. A graphics model of it is also shown using 
// Chai3D.

#include <Sai2Model.h>
#include <Sai2Simulation.h>
#include <Sai2Graphics.h>

#include <GLFW/glfw3.h> //must be loaded after loading opengl/glew

#include <iostream>
#include <string>

using namespace std;

const string world_file = "resources/world.urdf";
const string robot_file = "resources/pbot.urdf";
const string robot_name1 = "PBot1";
const string robot_name2 = "PBot2";
const string camera_name = "camera_fixed";

// callback to print glfw errors
void glfwError(int error, const char* description);

// callback when a key is pressed
void keySelect(GLFWwindow* window, int key, int scancode, int action, int mods);

int main() {
	cout << "Loading URDF world model file: " << world_file << endl;

	// load simulation world
	auto sim = new Simulation::Sai2Simulation(world_file, false);

	// load graphics scene
	auto graphics = new Sai2Graphics::Sai2Graphics(world_file, true);

	// load robots
	auto robot1 = new Sai2Model::Sai2Model(robot_file, false);
	auto robot2 = new Sai2Model::Sai2Model(robot_file, false);

	// offset a joint initial condition
	sim->getJointPositions(robot_name1, robot1->_q);
	sim->setJointPosition(robot_name1, 0, robot1->_q[0] + 0.5);
	
	/*------- Set up visualization -------*/
    // set up error callback
    glfwSetErrorCallback(glfwError);

    // initialize GLFW
    glfwInit();

    // retrieve resolution of computer display and position window accordingly
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    // information about computer screen and GLUT display window
	int screenW = mode->width;
    int screenH = mode->height;
    int windowW = 0.8 * screenH;
    int windowH = 0.5 * screenH;
    int windowPosY = (screenH - windowH) / 2;
    int windowPosX = windowPosY;

    // create window and make it current
    glfwWindowHint(GLFW_VISIBLE, 0);
    GLFWwindow* window = glfwCreateWindow(windowW, windowH, "03-sim_graphics_urdfmodel", NULL, NULL);
	glfwSetWindowPos(window, windowPosX, windowPosY);
	glfwShowWindow(window);
    glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

    // set callbacks
	glfwSetKeyCallback(window, keySelect);

    // while window is open:
    while (!glfwWindowShouldClose(window))
	{
		// update simulation by 1ms
		sim->integrate(0.01);

		// update kinematic models
		sim->getJointPositions(robot_name1, robot1->_q);
		sim->getJointVelocities(robot_name1, robot1->_dq);
		robot1->updateModel();
		sim->getJointPositions(robot_name2, robot2->_q);
		sim->getJointVelocities(robot_name2, robot2->_dq);
		robot2->updateModel();

		// update graphics. this automatically waits for the correct amount of time
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		graphics->updateGraphics(robot_name1, robot1);
		graphics->updateGraphics(robot_name2, robot2);
		graphics->render(camera_name, width, height);

		// swap buffers
		glfwSwapBuffers(window);

		// wait until all GL commands are completed
		glFinish();

		// check for any OpenGL errors
		GLenum err;
		err = glGetError();
		assert(err == GL_NO_ERROR);

	    // poll for events
	    glfwPollEvents();
	}

    // destroy context
    glfwDestroyWindow(window);

    // terminate
    glfwTerminate();

	return 0;
}

//------------------------------------------------------------------------------

void glfwError(int error, const char* description) {
	cerr << "GLFW Error: " << description << endl;
	exit(1);
}

//------------------------------------------------------------------------------

void keySelect(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // option ESC: exit
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        // exit application
         glfwSetWindowShouldClose(window, 1);
    }
}
