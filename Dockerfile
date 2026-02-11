# Stage 1: Build
FROM ubuntu:24.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential cmake qt6-base-dev qt6-httpserver-dev qt6-websockets-dev

COPY CMakeLists.txt /src/CMakeLists.txt
COPY main.cpp /src/main.cpp

RUN cmake -S /src -B /build && make -C /build -j$(nproc)

# Stage 2: Run
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libqt6httpserver6 libqt6core6t64 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/QtServer /app/server
COPY index.html /app/index.html

WORKDIR /app
EXPOSE 8080
CMD ["./server"]