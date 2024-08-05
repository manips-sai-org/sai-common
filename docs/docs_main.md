## Sai2-Common

Sai2-common contains a collection of utility classes to use with the other sai2 libraries. It provides the following:
* Implementation of a butterworth filter for eigen objects
* A c++ redis client based on hiredis to easily exchange data between programs (in particular eigen objects)
* A logger class to log data to text files
* A timer class to implement precisely timed loops 

### Examples
Look at the examples to see how to use the different classes in example applications.

To run one of the example, go to the corresponding subfolder of the build forlder, the binary name is the same as the example name:
```
cd build/examples/<XX-example_name>
./<XX-example_name>
``` 

* [01-timer](examples/01-timer.md)
* [02-filters](examples/02-filters.md)
* [03-logger](examples/03-logger.md)
* [04-redis_communication](examples/04-redis_communication.md)
* [05-timer_overtime_monitoring](examples/05-timer_overtime_monitoring.md)