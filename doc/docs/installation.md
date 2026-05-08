# 2. Installation

## 2.1 System requirements

- 64-bit Ubuntu 24.04 LTS or Debian 13 (trixie). Ubuntu 22.04 and older releases are not supported; other Linux distributions may work but are outside the supported configuration.
- CMake >= 3.16 and a C++20-capable GCC from the supported distribution. Clang currently fails due to template instantiation issues.
- Boost headers/libraries.
- [Microsoft SEAL](https://github.com/microsoft/SEAL) >= 4.1.0 installed system-wide. The Docker images build SEAL 4.3.2 by default, and a `libseal.a` symlink may be required for some manual builds.
- [Spot](https://spot.lre.epita.fr/) >= 2.4.3 for LTL parsing and automata construction.
- Optional: OCaml/opam for the Vehicle RSS example, bats for shell tests, and Doxygen/Graphviz for the developer manual.

Builds and encrypted runs can be resource intensive; bootstrapping-heavy runs may take several minutes and require 16 GB or more RAM.

## 2.2 Native installation on Ubuntu 24.04 or Debian 13

The following commands install the base native-build dependencies used by the Dockerfiles. Run them on a fresh Ubuntu 24.04 or Debian 13 system. Spot and the Vehicle RSS-only OCaml/opam dependency are listed separately below.

### 2.2.1 Install system packages

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  bash \
  bats \
  bison \
  build-essential \
  ca-certificates \
  cmake \
  curl \
  file \
  flex \
  gawk \
  git \
  gpg \
  libboost-all-dev \
  libgmp-dev \
  libtbb-dev \
  libzstd-dev \
  m4 \
  pkg-config \
  python3 \
  texinfo \
  wget \
  xz-utils \
  zlib1g-dev
```

### 2.2.2 Install Microsoft GSL

```sh
cd /tmp
wget -O msgsl.tar.gz https://github.com/microsoft/GSL/archive/refs/tags/v4.0.0.tar.gz
tar -xzf msgsl.tar.gz
cmake -S GSL-4.0.0 -B GSL-4.0.0/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DGSL_TEST=OFF
cmake --build GSL-4.0.0/build
sudo cmake --install GSL-4.0.0/build
rm -rf GSL-4.0.0 msgsl.tar.gz
```

### 2.2.3 Install Microsoft SEAL

```sh
cd /tmp
wget -O seal.tar.gz https://github.com/microsoft/SEAL/archive/refs/tags/v4.3.2.tar.gz
tar -xzf seal.tar.gz
cmake -S SEAL-4.3.2 -B SEAL-4.3.2/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DSEAL_BUILD_EXAMPLES=OFF \
  -DSEAL_BUILD_TESTS=OFF \
  -DSEAL_BUILD_BENCH=OFF \
  -DSEAL_BUILD_SEAL_C=OFF \
  -DSEAL_USE_ZLIB=ON \
  -DSEAL_USE_ZSTD=ON \
  -DSEAL_BUILD_DEPS=OFF
cmake --build SEAL-4.3.2/build
sudo cmake --install SEAL-4.3.2/build
sudo ldconfig
rm -rf SEAL-4.3.2 seal.tar.gz
```

If your linker later reports that it cannot find `libseal.a`, create the compatibility symlink used by some manual builds:

```sh
sudo ln -s /usr/local/lib/libseal-4.3.a /usr/local/lib/libseal.a
sudo ldconfig
```

### 2.2.4 Install Spot

Spot packages are distributed through Spot's own APT repositories, so configure the appropriate repository before installing `spot` and `libspot-dev`. The commands below follow the [official Spot installation instructions](https://spot.lre.epita.fr/install.html).

On Debian 13 (trixie):

```sh
sudo install -d -m 0755 /etc/apt/keyrings
sudo wget -q -O /etc/apt/keyrings/lre-epita.gpg https://www.lre.epita.fr/repo/debian.gpg
echo "deb [signed-by=/etc/apt/keyrings/lre-epita.gpg] http://www.lre.epita.fr/repo/debian/ stable/" \
  | sudo tee /etc/apt/sources.list.d/lre-epita.list
sudo apt-get update
sudo apt-get install -y --no-install-recommends spot libspot-dev
```

On Ubuntu 24.04:

```sh
sudo install -d -m 0755 /etc/apt/keyrings
wget -q -O - 'https://build.opensuse.org/projects/home:adl/signing_keys/download?kind=gpg' \
  | sudo gpg --dearmor --yes -o /etc/apt/keyrings/home-adl-obs.gpg
echo 'deb [signed-by=/etc/apt/keyrings/home-adl-obs.gpg] https://download.opensuse.org/repositories/home:/adl/xUbuntu_24.04/ ./' \
  | sudo tee /etc/apt/sources.list.d/home-adl-obs.list
sudo apt-get update
sudo apt-get install -y --no-install-recommends spot libspot-dev
```

Verify that both Spot libraries required by ArithHomFA are visible to `pkg-config`:

```sh
pkg-config --modversion libspot
pkg-config --modversion libbddx
```

## 2.3 Build ArithHomFA from source

```sh
git clone https://github.com/MasWag/arith_homfa.git
cd arith_homfa
git submodule update --init --recursive
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
```

To build the bundled examples as well, run:

```sh
cmake -S examples -B examples/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build examples/build
```

If you only need the command-line utility and runner library, build those targets explicitly:

```sh
cmake --build cmake-build-release --target ahomfa_util ahomfa_runner
```

## 2.4 Optional Vehicle RSS example dependency

The Vehicle RSS script (`examples/run_vrss.sh`) generates `examples/vehicle_rss/vrss.ltl` from `vrss.xltl` with the bundled `thirdparty/ltlconv` tool. That path requires OCaml/opam; the base build and the blood-glucose example do not.

Install and initialize opam before running the Vehicle RSS example:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends ocaml opam
opam init --bare --disable-sandboxing -y
opam switch create default ocaml-system -y
eval "$(opam env)"
```

If you already have a default opam switch, just run:

```sh
eval "$(opam env)"
```

Then run the example from the repository root:

```sh
./examples/run_vrss.sh
```

The example Makefile installs the `ltlconv` opam dependencies and builds `ltlconv` automatically when it needs to regenerate `vrss.ltl`.

## 2.5 Docker installation

Prebuilt images are available on Docker Hub, so normal users do not need to build images locally. Docker will pull an image automatically the first time you run it, or you can pull it explicitly:

```sh
docker pull maswag/arith_homfa:noble
docker pull maswag/arith_homfa:trixie
```

Run an interactive shell in the Ubuntu 24.04 image:

```sh
docker run --rm -it maswag/arith_homfa:noble
```

Use `maswag/arith_homfa:trixie` in the same command to run the Debian 13 image.

Build images locally only when you need to modify the Dockerfiles or test a different dependency version:

```sh
./docker/build_images.sh
```

The Docker images already include the Vehicle RSS OCaml/opam tooling.

## 2.6 Sanity checks

### 2.6.1 Native build

After building, run:

```sh
./cmake-build-release/ahomfa_util --help
```

At least one tutorial should run end-to-end as described in the Quick Start section. For example:

```sh
./examples/run_bg.sh --formula 1 --mode fast
```

This doubles as a sanity check that the CKKS, TFHE, SEAL, Spot, and example dependencies are configured correctly.

### 2.6.2 Docker image

Run the command-line utility inside the published Ubuntu image:

```sh
docker run --rm maswag/arith_homfa:noble ahomfa_util --help
```

Run the blood-glucose smoke test inside the image:

```sh
docker run --rm -it maswag/arith_homfa:noble ./examples/run_bg.sh --formula 1 --mode fast
```

Use `maswag/arith_homfa:trixie` in the same commands for the Debian 13 image.
