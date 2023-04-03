# CHANGELOG

Release Versions:

- [1.1.0](#110)
- [1.0.0](#100)
- [0.2.0](#020)
- [0.1.0](#010)

## 1.1.0

Version 1.1.0 sets control libraries version 6.0.0 as default and fixes the GitHub workflow. Additionally, the C++
library now has its proper CMake configuration and can be included and linked with `find_package(network_interfaces)`
and`target_link_libraries(${MY_TARGET} ${network_interfaces_LIBRARIES}`.

## 1.0.0

Version 1.0.0 accounts for breaking changes in control libraries and defines 5.0.0 as the required control libraries
version. Additionally, there is now an install script that takes care of the installation of network interfaces.

## 0.2.0

Version 0.2.0 introduces a slightly different Dockerfile and contains a GitHub workflow to build and push an image with
the pre-built network interface libraries. Additionally, two loopback scripts have been added to simplify communication
testing.

## 0.1.0

A set of drivers, protocols and libraries for communicating between software and hardware devices.

### Features

**ZMQ Interface**

A network interface with ZMQ subscribers and publishers, implemented as a library in both C++ and Python. The library
defines the message structures using `state_representation` types and the encoding / decoding of the messages
using `clproto`.
