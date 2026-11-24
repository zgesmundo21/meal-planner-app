# Use Ubuntu as the Linux operating system.
FROM ubuntu:24.04

# Prevent interactive installation prompts.
ENV DEBIAN_FRONTEND=noninteractive

# Install the tools required to compile the C++ project.
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg.
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg

RUN /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

# Configure vcpkg.
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="/opt/vcpkg:${PATH}"

# Set the application working directory.
WORKDIR /app

# Copy the GitHub project into the container.
COPY . .

# Configure the C++ project.
RUN cmake -S . -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux

# Compile the MealMate web server.
RUN cmake --build build --target MealMateServer --parallel 2

# Set the default server port.
ENV PORT=10000

EXPOSE 10000

# Start the MealMate server.
CMD ["./build/MealMateServer"]