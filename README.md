# JSDB
A masterless distributed in-memory key-value database (distributed hash table)

## Features
- Highly calable and available
- Fault tolerance through replication
- IO Concurrency on each node through asynchronous IO implemented by Boost.Asio
- Gossip-stype membership protocol
- Distributes keys evenly in the cluster through consistant hashing algorithm
- Atomic write operation across multiple nodes through a two-phase commit protocol
- Achieves eventual consistency by a push-based stabilization  protocol

## Dependencies
On Linux (I'm using Ubuntu 16.04.6), install all the dependencies
```
gcc (needs at least c++14)
Boost (1.66)
pthread
protobuf (3.21.2)
```
-------------------------------------------------------
## Build
```bash
make  #build server
make client #build client
make proto  #compile protobuf .message file to cpp
make clean  #clean binaries
make deepclean #clean binaries + precompiled header file
```
--------------------------------------------------------
## Server Usage
#### Start server node
```bash
# start a bootstrap node (first node in the group)
dhserver <id> <ip> <port> <ring_id>

# add a new node to the group created by the bootstrap node at <bootstrap_ip>:<bootstrap_port>
dhserver <id> <ip> <port> <ring_id> <bootstrap_ip> <bootstrap_port>
```

#### Example
On a machine with ip address = 192.168.40.129
```bash
# start a node with id=0, self_ip=192.168.40.129, port=9000
# ring_id=0 (for consistant hashing, default ring size = 65536)
# start the server as the bootstrap server (the first server in the cluster)
bin/dhserver 0 192.168.40.129 9000 0
```
In another terminal
```bash
# start another node with id=1, self_ip=192.168.40.129, port=9001, ring_id=30000
# tell it to join the group created by the bootstrap server at 192.168.40.129:9000
bin/dhserver 1 192.168.40.129 9001 30000 192.168.40.129 9000
```
--------------------------------------
## Client Usage
#### Start a client
```bash
# connect to a node at ip:port
client <ip> <port>
```
```bash
# connect to the node at 192.168.40.129:9000 (alternatively you can also connect to the one at port 9001)
bin/client 192.168.40.129 9000
```
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
-----------------------------------------

## Scripts
There are some scripts to help you easily create a cluster for testing.
Note that you need to edit the ip addresses in the start-cluster-* scripts for them to work.
```
script/start-cluster-3.sh # start a cluster of size 3
script/start-cluster-10.sh # start a cluster of size 10
script/status.sh        # ps all the dhserver nodes
script/stop-cluster.sh  # stop all dhserver nodes
```


## Full Example
Suppose the local ip address is 192.168.40.129 in the following example (you need to change it to your own ip address, or 127.0.0.1)
```
$ script/start-cluster-10.sh
$ script/status.sh
# you should see 10 nodes and their pids.
$ bin/client 192.168.40.129 9002
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

# you can now kill node at port 9002 (and maybe some other nodes)

$ bin/client 192.168.40.129 9005
> get hello
world
> get k1
v1
> get k2
v2
> get k3
v3
> exit
$ script/stop-cluster.sh
$
```
--------------------------------------------------

## Configuration
check `struct DHConst` in `utils.hpp`

## TODO
- Use a config file instead of hard code the constants and command line args
