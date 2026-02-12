# Creating an ubuntu image to build the program in
FROM ubuntu:24.04 AS builder
# This prevents the package manager from asking us stuff which docker (an automatic system) can't handle
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary packages
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    qt6-httpserver-dev \
    qt6-websockets-dev \
    && rm -rf /var/lib/apt/lists/* # Cleanup

WORKDIR /src

# Copying the source files to the image
COPY CMakeLists.txt .
COPY src/ ./src/
COPY include/ ./include/

# Configure build
RUN cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Debug
# Actually build (in parallel to use all CPU cores)
RUN cmake --build /build --parallel $(nproc)

# Now we create the image that will actually run the server. The build image will be discarded.
FROM ubuntu:24.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

# Install the necessary libraries to run the server, excluding the ones only needed to build it.
RUN apt-get update && apt-get install -y --no-install-recommends \
    libqt6httpserver6 \
    libqt6core6t64 \
    libqt6websockets6 \
    && rm -rf /var/lib/apt/lists/*

# Creates a new user to run the server
RUN useradd --system --no-create-home appuser

WORKDIR /app

# Copies just the final binary from the builder image to the runner one
COPY --from=builder /build/smvcar-server ./smvcar-server
# Also copy the webpage itself
COPY index.html .

# Give the user ownership of everything in the image
RUN chown -R appuser:appuser /app
# Then switch the active user to said user
USER appuser

# The port we use for connecting to the client
EXPOSE 8080

# Actually run the server program
CMD ["./smvcar-server"]