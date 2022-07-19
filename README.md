# JSDB
A minimal masterless distributed in-memory key-value database (distributed hash table)

## Features
- Scalable and highly available
- IO Concurrency on each node through Boost.Asio
- Gossip-stype membership protocol
- Distributing keys evenly in the cluster through a Gossip-style membership protocol
- Atomic write operation through the two-phase commit protocol
- Eventual Consistency achieved by letting each node periodically pushing some records to their neighbors

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

## Configuration
check `struct DHConst` in `utils.hpp`

## TODO
- Use a config file instead of hard code the constants