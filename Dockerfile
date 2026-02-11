# --- STAGE 1: Build the Server ---
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install dependencies (These rarely change, so they will be CACHED)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-httpserver-dev \
    qt6-websockets-dev

# 2. Copy ONLY the build config first
WORKDIR /build
COPY CMakeLists.txt /src/CMakeLists.txt

# 3. Pre-configure CMake (This creates the Makefiles, usually cached if CMakeLists doesn't change)
RUN cmake -S /src -B /build

# 4. NOW copy the actual source code (This changes frequently!)
COPY main.cpp /src/main.cpp
COPY index.html /src/index.html

# 5. Compile (Only this part runs when you change code)
RUN make -j$(nproc)

# --- STAGE 2: Run the Server ---
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime libs (Cached unless you change this list)
RUN apt-get update && apt-get install -y \
    libqt6httpserver6 \
    libqt6websockets6 \
    libqt6core6t64 \
    shared-mime-info \
    && update-mime-database /usr/share/mime \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/QtServer /app/server
COPY index.html /app/index.html

WORKDIR /app
EXPOSE 8080
CMD ["./server"]