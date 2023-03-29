# network-interfaces

A repository of drivers, protocols and libraries for communicating between software and hardware devices.

## License

The code is from https://github.com/aica-technology/network-interfaces 


## Installation and usage

This project depends on the `state_representation` and `clproto` libraries from
[control libraries](https://github.com/aica-technology/control-libraries) in C++ as well as Python. Before installing the
network interfaces libraries, install therefore the two modules mentioned above as well as their Python bindings.
Afterwards, the installation is straight forward:

```console
[sudo] bash install.sh -y
```

Run `bash install.sh -h` for help with this process.

### ZMQ loopback scripts

Both the C++ and Python directory contain *loopback* executables that subscribe to the state (or command)
and publish a command (or state) in and endless loop. They can be used to check if the robot and the user are connected
and receive each other's messages correctly. To run the scripts, make the CMake project and/or install the Python
project, and then choose one of the following:

```
cd path/to/cpp/build
./zmq_loopback_state state_uri command_uri
./zmq_loopback_command state_uri command_uri

cd path/to/python/scripts
python3 zmq_loopback_state.py state_uri command_uri
python3 zmq_loopback_command.py state_uri command_uri
```

Note that the scripts are provided with a correct combination of state and command URIs. There are examples of such
combinations below. Assume the robot state is on port 1601 and the command on 1602:

#### Everything runs in one container / on the same host (network independent)

If all applications run in the same container, or on the same host, the situation is:

- The robot publishes its state on `0.0.0.0:1601` and listens to commands on `0.0.0.0:1602` with both sockets
  non-binding: run `./zmq_loopback_state *:1601 *:1602` or `python3 zmq_loopback_state.py *:1601 *:1602` to receive and
  print the robot's state.
- The controller sends the command on `*:1602` and receives the state on `*:1601` with both sockets binding:
  run `./zmq_loopback_command 0.0.0.0:1601 0.0.0.0:1602` or
  `python3 zmq_loopback_command.py 0.0.0.0:1601 0.0.0.0:1602` to receive and print the command and send back a random
  state.

#### One or more containers and host, all on host network and with no hostname

Same as above.

#### One container with host (container not on host network)

The container is an SSH server or needs to be on a user-defined network, but the robot is connected directly to the host
machine. This is almost the same case as above:

- The controller sends the command on `*:1602` and receives the state on `*:1601` with both sockets binding:
  run `./zmq_loopback_command 0.0.0.0:1601 0.0.0.0:1602` or
  `python3 zmq_loopback_command.py 0.0.0.0:1601 0.0.0.0:1602` to receive and print the command and send back a random
  state.

There is one important difference though: The container needs to bind ports 1601 and 1602 (i.e.
add `-p1601:1601 -p1602:1602` to the `docker run` command) explicitly such that the communication goes through.

#### Several containers, user-defined bridge network with hostnames

If the containers all run on a user-defined bridge network, the connecting sockets need to be provided with the hostname
of the binding sockets. For example, if the containers are running on network *aicanet* and have hostnames *robot* and
*controller*, respectively.

- The robot publishes its state on `controller.aicanet:1601` and listens to commands on `controller.aicanet:1602` with
  both sockets non-binding: run `./zmq_loopback_state *:1601 *:1602` or `python3 zmq_loopback_state.py *:1601 *:1602` to
  receive and print the robot's state.
- The controller sends the command on `*:1602` and receives the state on `*:1601` with both sockets binding:
  run `./zmq_loopback_command controller.aicanet:1601 controller.aicanet:1602` or `
  python3 zmq_loopback_command.py controller.aicanet:1601 controller.aicanet:1602` to receive and print the command and
  send back a random state.

#### Note

- This list of combinations is not exhaustive.
- The binding sockets always have a URI like `*:port` whilst the connecting sockets need to provide the complete address
  version (`0.0.0.0:port` if on localhost or `hostname.network:port` if on bridge network).

## Development

Build and run a Docker container as an SSH toolchain server for remote development with:

```console
bash dev-server.sh
```

Note: This requires the installation of the `aica-docker` scripts
from [here](https://github.com/aica-technology/docker-images).

Additionally, to run the tests without an SSH toolchain server, build an image with:

```console
bash build-test.sh
```
