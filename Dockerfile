# --- STAGE 1: Build the Server (Heavy Lifting) ---
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install dependencies (This layer is CACHED and won't re-run)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-httpserver-dev \
    qt6-websockets-dev

WORKDIR /build

# 2. Copy Source Code
# We copy CMakeLists.txt AND main.cpp here.
# CMake needs main.cpp to exist before it can configure the project.
COPY CMakeLists.txt /src/CMakeLists.txt
COPY main.cpp /src/main.cpp

# 3. Configure and Compile
RUN cmake -S /src -B /build
RUN make -j$(nproc)

# --- STAGE 2: Run the Server (Lightweight) ---
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime libs
RUN apt-get update && apt-get install -y \
    libqt6httpserver6 \
    libqt6websockets6 \
    libqt6core6t64 \
    shared-mime-info \
    && update-mime-database /usr/share/mime \
    && rm -rf /var/lib/apt/lists/*

# Copy the binary from the builder
COPY --from=builder /build/QtServer /app/server

# Copy the HTML file ONLY here.
# Benefit: Changing HTML does NOT trigger a C++ recompile!
COPY index.html /app/index.html

WORKDIR /app
EXPOSE 8080
CMD ["./server"]