FROM debian:trixie

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

ARG DEBIAN_FRONTEND=noninteractive
ARG SEAL_VERSION=4.1.1

# Spot publishes Debian stable packages for trixie.
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      ca-certificates \
      curl \
      gpg \
      wget && \
    install -d -m 0755 /etc/apt/keyrings && \
    wget -q -O /etc/apt/keyrings/lre-epita.gpg https://www.lre.epita.fr/repo/debian.gpg && \
    echo 'deb [signed-by=/etc/apt/keyrings/lre-epita.gpg] http://www.lre.epita.fr/repo/debian/ stable/' \
      > /etc/apt/sources.list.d/lre-epita.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
      bash \
      bats \
      bison \
      build-essential \
      cmake \
      file \
      flex \
      gawk \
      git \
      libboost-all-dev \
      libgmp-dev \
      libmsgsl-dev \
      libspot-dev \
      libtbb-dev \
      m4 \
      make \
      ninja-build \
      pkg-config \
      python3 \
      spot \
      texinfo \
      xz-utils \
      zlib1g-dev \
      libzstd-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

# Build and install Microsoft SEAL from source.
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
    cmake --build "SEAL-${SEAL_VERSION}/build" -j"$(nproc)" && \
    cmake --install "SEAL-${SEAL_VERSION}/build" && \
    ldconfig && \
    rm -rf "SEAL-${SEAL_VERSION}" seal.tar.gz

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
CMD ["/bin/bash"]
