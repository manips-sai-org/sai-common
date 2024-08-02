## Project name: sai2-common (version: 0.1.0)
Note: This project uses Semantic Versioning (http://semver.org/).

### Documentation
The documentation can be generated locally with doxygen:
```
cd docs
doxygen
```

It can also be accessed online [Here](https://manips-sai-org.github.io/sai2-common/)

### Project description:
The Sai2-Common library implements a set of utility functionalities to simplify robot simulation and control using SAI. This includes the following modules:

* __filters__ module: to filter signals using eigen library
* __force_sensor__ module: to simulate a force sensor when using sai2-simulation
* __redis__ module: redis functions with eigen objects to communicate data between programs
* __timer__ module: to implement a timer and regulate the rate of a control loop
* __uiforce__ module: to apply a force with the mouse when using sai2

### 3rdParty dependencies (* = installation required):

sudo apt install libhiredis-dev libjsoncpp-dev

* Redis*: Redis server [brew, apt-get]
* Hiredis*: Redis minimalist client [brew, apt-get]
* JsonCpp*: JSON serialization [brew, apt-get]

### SAI library dependencies:
* sai2-simulation: Articulated rigid-body physics simulation [private at https://github.com/manips-sai/sai2-simulation, contact shameekg@stanford.edu]
* sai2-model: Articulated rigid-body model [https://github.com/manips-sai-org/sai2-model]
* sai2-graphics: To display simulation [https://github.com/manips-sai-org/sai2-graphics]

### Installation instructions:
```
mkdir build
cd build
cmake .. && make -j4
```

### Uninstallation instructions: 

1. rm -r build
2. rm -r ~/.cmake/packages/SAI2-COMMON

### Getting started:
Take a look at sample applications under examples/.
You can run them from build/examples/<x-example>/.

### License:
Currently pending licensing. PLEASE DO NOT DISTRIBUTE.

### Project contributors:
* Gerald Brantner
* Brian Soe
* Mikael Jorda
* Shameek Ganguly
* Toki Migimatsu

### For questions, contact:
manips.sai@gmail.com  
mjorda@stanford.edu  
shameekg@stanford.edu
