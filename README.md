## Sai-Common
The Sai-Common library implements a set of utility functionalities to simplify robot simulation and control using SAI.

### Documentation
The documentation can also be accessed online at the following [link](https://manips-sai-org.github.io/sai-common/)

It can also be generated locally with doxygen:
```
cd docs
doxygen
```

### 3rdParty dependencies (* = installation required):

sudo apt install libhiredis-dev libjsoncpp-dev

* Redis*: Redis server [brew, apt-get]
* Hiredis*: Redis minimalist client [brew, apt-get]
* JsonCpp*: JSON serialization [brew, apt-get]

### Installation instructions:
```
mkdir build
cd build
cmake .. && make -j4
```

### Uninstallation instructions: 

1. rm -r build
2. rm -r ~/.cmake/packages/SAI-COMMON

### Getting started:
Take a look at sample applications under examples/.
You can run them from build/examples/<x-example>/.
More details on the examples [here](./docs/docs_main.md)

### License:
Currently pending licensing. PLEASE DO NOT DISTRIBUTE.

### For questions, contact:
mjorda@stanford.edu
