## Redis communication example

This example shows how to use the RedisClient. It created 2 threads and communicates between the two using redis.

Note that if you get an error when launching it of the type:

*terminating due to uncaught exception of type std::runtime_error: RedisClient: Could not connect to redis server: Connection refused*

It means the redis server is not running. If it is the case, open a new terminal and type
```
redis-server
```