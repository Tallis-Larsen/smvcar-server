# --- STAGE 1: Build the Server ---
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-httpserver-dev \
    qt6-websockets-dev

COPY . /src
WORKDIR /build

RUN cmake /src && make

# --- STAGE 2: Run the Server ---
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime libraries AND the shared-mime-info utility
RUN apt-get update && apt-get install -y \
    libqt6httpserver6 \
    libqt6websockets6 \
    libqt6core6t64 \
    shared-mime-info

# FIX: Explicitly generate the mime-type database.
# Without this, QHttpServer might not know that "index.html" is "text/html"
# and will refuse to serve it.
RUN update-mime-database /usr/share/mime

COPY --from=builder /build/QtServer /app/server
COPY index.html /app/index.html

WORKDIR /app

EXPOSE 8080

CMD ["./server"]