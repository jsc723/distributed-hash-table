# JSDB
A minimal masterless distributed in-memory key-value database (distributed hash table)

## Features
- Scalable and highly available
- Fault tolerance through replication
- IO Concurrency on each node through Boost.Asio
- Gossip-stype membership protocol
- Distributing keys evenly in the cluster through consistant hashing
- Atomic write operation through the two-phase commit protocol
- Eventual Consistency achieved by letting each node periodically pushing records to neighbors

## Dependencies
On Linux (I'm using Ubuntu 16), install all the dependencies
```
gcc (needs at least c++14)
Boost (1.66)
pthread
protobuf (3.21.2)
```

## Build
```bash
make  #build server
make client #build client
make proto  #compile protobuf .message file to cpp
make clean  #clean binaries
make deepclean #clean binaries + precompiled header file
```

## Run
```bash
bin/dhserver 0 9000 0  # start a single node with id=0, port=9000
                       # ring_position=0 (for consistant hashing, default ring size = 65536)
bin/client 9000        # connect to the node at port=9000
script/start-cluster-10.sh # start a cluster of size 10
script/stop-cluster.sh  # stop all dhserver instances
```

Note: currently all nodes runs on the same machine with different port number, but it is easy to 
extend to multiple machines.

## Usage
First, start a cluster and then connect to a node in the cluster using client

#### set a key to a value
```
set <key> <value>
```

#### get value of a key
```
get <key>
```

#### exit client
```
exit
```

## Example
```
$ script/start-cluster-10.sh
$ bin/client 9002
> set k1 v1
ok
> set k2 v2
ok
> set k3 v3
ok
> set hello world
ok
> get k1
v1
> get k2
v2
> exit
$ bin/client 9005
> get hello
world
> get k1
v1
> get k3
v3
> exit
$ script/stop-cluster.sh
$
```


## Configuration
check `struct DHConst` in `utils.hpp`

## TODO
- Support multiple machines
- Use a config file instead of hard code the constants
