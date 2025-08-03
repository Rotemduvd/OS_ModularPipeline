FROM ubuntu:24.04

# Install necessary tools
RUN apt-get update && \
    apt-get install -y build-essential gcc-13 g++-13 gdb make cmake git vim && \
    apt-get install -y manpages-dev && \
    apt-get clean

# Set gcc and g++ 13 as default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Install pthreads and dl (already part of glibc)
RUN apt-get install -y libpthread-stubs0-dev

# Set workdir
WORKDIR /workspace