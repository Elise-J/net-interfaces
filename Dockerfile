# FROM ghcr.io/aica-technology/control-libraries/development-dependencies as source-dependencies

FROM ghcr.io/aica-technology/ros-control-libraries:noetic as source-dependencies


ARG INSTALL_DESTINATION=${USER}/source

RUN sudo apt-get update && sudo apt-get install -y \
  featherpad \
  libzmq3-dev \
  nano
RUN sudo apt-get clean 
RUN sudo rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
ARG CPPZMQ_VERSION=4.7.1
RUN wget https://github.com/zeromq/cppzmq/archive/v${CPPZMQ_VERSION}.tar.gz -O cppzmq-${CPPZMQ_VERSION}.tar.gz
RUN tar -xzf cppzmq-${CPPZMQ_VERSION}.tar.gz
WORKDIR /tmp/cppzmq-${CPPZMQ_VERSION}
RUN mkdir build && cd build && cmake .. -DCPPZMQ_BUILD_TESTS=OFF && sudo make -j install

WORKDIR /tmp
ARG CONTROL_LIBRARIES_BRANCH=v7.0.0
RUN git clone -b ${CONTROL_LIBRARIES_BRANCH} --depth 1 https://github.com/aica-technology/control-libraries.git
RUN cd control-libraries/source && sudo ./install.sh --auto --no-controllers --no-dynamical-systems --no-robot-model
RUN cd control-libraries/protocol && sudo ./install.sh --auto
RUN pip3 install control-libraries/python

RUN sudo rm -rf /tmp/*

# install pyzmq
RUN pip3 install pyzmq

# FROM source-dependencies as build-test

# WORKDIR ${HOME}
# COPY --chown=${USER} ./cpp ./network_interfaces/cpp
# COPY --chown=${USER} ./python ./network_interfaces/python

# RUN cd ./network_interfaces/cpp && mkdir build && cd build && cmake -DBUILD_TESTING=ON .. \
#   && make -j && CTEST_OUTPUT_ON_FAILURE=1 make test && make -j install
# RUN cd ./network_interfaces/python && pip3 install ./ && python3 -m unittest discover ./test --verbose

WORKDIR ${HOME}
RUN git clone https://github.com/Elise-J/net-interfaces
WORKDIR ${HOME}/net-interfaces/cpp
RUN mkdir build 
WORKDIR ${HOME}/net-interfaces/cpp/build 
RUN cmake -DBUILD_TESTING=ON .. && make -j && CTEST_OUTPUT_ON_FAILURE=1 make test && sudo make -j install
# RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON .. \
#       -DCMAKE_INSTALL_PREFIX=${INSTALL_DESTINATION} .. 
# RUN make -j && make install
WORKDIR ${HOME}/net-interfaces/python 
RUN pip3 install ./ && python3 -m unittest discover ./test --verbose

# Clean image
RUN rm -rf ./network_interfaces
