FROM ubuntu:noble

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Etc/UTC \
    SEAL_VERSION=4.1.1 \
    SPOT_VERSION=2.10.4 \
    MSGSL_VERSION=4.0.0

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      bash \
      bats \
      bison \
      build-essential \
      ca-certificates \
      cmake \
      file \
      flex \
      gawk \
      git \
      libboost-all-dev \
      libgmp-dev \
      libtbb-dev \
      m4 \
      make \
      ninja-build \
      pkg-config \
      python3 \
      texinfo \
      wget \
      xz-utils \
      zlib1g-dev \
      libzstd-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

RUN wget -O msgsl.tar.gz "https://github.com/microsoft/GSL/archive/refs/tags/v${MSGSL_VERSION}.tar.gz" && \
    tar -xzf msgsl.tar.gz && \
    cmake -S "GSL-${MSGSL_VERSION}" -B "GSL-${MSGSL_VERSION}/build" \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DGSL_TEST=OFF && \
    cmake --build "GSL-${MSGSL_VERSION}/build" && \
    cmake --install "GSL-${MSGSL_VERSION}/build" && \
    rm -rf "GSL-${MSGSL_VERSION}" msgsl.tar.gz

RUN wget -O seal.tar.gz "https://github.com/microsoft/SEAL/archive/refs/tags/v${SEAL_VERSION}.tar.gz" && \
    tar -xzf seal.tar.gz && \
    cmake -S "SEAL-${SEAL_VERSION}" -B "SEAL-${SEAL_VERSION}/build" \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DSEAL_BUILD_EXAMPLES=OFF \
      -DSEAL_BUILD_TESTS=OFF \
      -DSEAL_BUILD_BENCH=OFF \
      -DSEAL_BUILD_SEAL_C=OFF \
      -DSEAL_USE_ZLIB=ON \
      -DSEAL_USE_ZSTD=ON \
      -DSEAL_BUILD_DEPS=OFF && \
    cmake --build "SEAL-${SEAL_VERSION}/build" && \
    cmake --install "SEAL-${SEAL_VERSION}/build" && \
    ldconfig && \
    rm -rf "SEAL-${SEAL_VERSION}" seal.tar.gz

RUN wget -O spot.tar.gz "https://www.lre.epita.fr/dload/spot/spot-${SPOT_VERSION}.tar.gz" && \
    tar -xzf spot.tar.gz && \
    cd "spot-${SPOT_VERSION}" && \
    ./configure --prefix=/usr/local --disable-python && \
    make -j"$(nproc)" && \
    make install && \
    ldconfig && \
    cd /tmp && \
    rm -rf "spot-${SPOT_VERSION}" spot.tar.gz

# Clone and build arith_homfa
RUN mkdir /arith_homfa
WORKDIR /arith_homfa
RUN git init && \
    git remote add origin https://github.com/MasWag/arith_homfa.git && \
    git fetch --depth 1 origin d78134551ec03f20038d83a42ab04cda4eae7df1 && \
    git checkout FETCH_HEAD && \
    git submodule update --recursive --init && \
    cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release && \
    cmake --build cmake-build-release -j"$(nproc)" && \
    cmake -S examples -B examples/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && \
    cmake --build examples/build -j"$(nproc)"


ENV PATH="/arith_homfa/cmake-build-release:/arith_homfa/examples:/arith_homfa/examples/build/vehicle_rss:/arith_homfa/examples/build/blood_glucose:${PATH}"
WORKDIR /arith_homfa
CMD ["/bin/bash"]
