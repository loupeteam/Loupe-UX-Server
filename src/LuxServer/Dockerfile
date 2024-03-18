FROM ubuntu:22.04

# Non-interactive installation mode
ENV DEBIAN_FRONTEND=noninteractive

# Install essentials
RUN apt-get update && apt-get install -y apt-utils build-essential git gitk cmake curl gdb
RUN apt-get install ninja-build -y
RUN apt-get install nodejs npm -y

# Install ASIO
RUN apt-get update && apt-get install -y libasio-dev

RUN mkdir /depends/

# Install Crow
RUN cd /depends/ \
    && git clone https://github.com/CrowCpp/Crow.git\
    && cd Crow \
    && mkdir build && cd build \
    && cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF \
    && make install

ENV CMAKE_GENERATOR "Ninja"

# Set the working directory
COPY . /src

# # Install ADS
RUN cd /src/depends/ADS \
    && mkdir build && cd build \
    && cmake .. \
    && ninja install

# # Build lux server
RUN cd /src/ \
    && mkdir build_ && cd build_ \
    && cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    && ninja install

CMD ["bash"]