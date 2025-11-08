FROM emscripten/emsdk:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    python3 \
    ca-certificates \
    curl \
    pkg-config \
    autoconf \
    automake \
    libtool \
    patch \
    ninja-build \
    cmake \
    xa65 \
    unzip \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN emcc --version

COPY bindings.cpp /opt/libsidplayfp-wasm/bindings.cpp
COPY entrypoint.sh /opt/libsidplayfp-wasm/entrypoint.sh
COPY scripts /opt/libsidplayfp-wasm/webassembly/scripts

RUN chmod +x /opt/libsidplayfp-wasm/entrypoint.sh

WORKDIR /opt/libsidplayfp-wasm

ENTRYPOINT ["/opt/libsidplayfp-wasm/entrypoint.sh"]
