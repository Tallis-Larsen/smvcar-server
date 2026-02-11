# --- STAGE 1: Build the Server ---
# We use Ubuntu 24.04 because it has Qt 6.4+ (required for QHttpServer) in its default repos.
FROM ubuntu:24.04 AS builder

# Prevent interactive prompts during install
ENV DEBIAN_FRONTEND=noninteractive

# Install C++ compiler, CMake, and Qt6 development files
# FIX: Added qt6-websockets-dev because Qt6HttpServer depends on it for configuration
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-httpserver-dev \
    qt6-websockets-dev

# Copy source code into the container
COPY . /src
WORKDIR /build

# Compile the project
RUN cmake /src && make

# --- STAGE 2: Run the Server ---
# Create a fresh, smaller image with only the runtime libraries needed
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime libraries
# FIX: Added libqt6websockets6 which is required by libqt6httpserver6
RUN apt-get update && apt-get install -y \
    libqt6httpserver6 \
    libqt6websockets6 \
    libqt6core6t64

# Copy the compiled binary from the builder stage
COPY --from=builder /build/QtServer /app/server
# Copy the HTML file so the server can find it
COPY index.html /app/index.html

WORKDIR /app

# DigitalOcean needs to know we are using port 8080
EXPOSE 8080

# Run the server
CMD ["./server"]