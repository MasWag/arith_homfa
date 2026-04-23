# Docker support for ArithHomFA

This directory contains Dockerfiles for the supported ArithHomFA operating systems: Ubuntu 24.04 LTS and Debian 13 (trixie). Ubuntu 22.04 is not supported.

## Files

- `docker/build_images.sh` - Script to build all supported Docker images
- `docker/ubuntu24.04/Dockerfile`
- `docker/debian-trixie/Dockerfile`

Both images:

- start from the official `ubuntu:noble` or `debian:trixie` image,
- install build dependencies with `apt`,
- build and install Microsoft SEAL 4.1.1,
- install or build Spot,
- copy the repository into the image, and
- build the repository with CMake/Ninja.

## Important note about submodules

The build expects the repository submodules to be present in the Docker build context.
Before building the image, run:

```sh
git submodule update --init --recursive
```

## Build commands

Quick build using the provided script:

```sh
./build_images.sh
```

Manual build:

```sh
docker build -f docker/ubuntu24.04/Dockerfile -t maswag/arith_homfa:noble .
docker build -f docker/debian-trixie/Dockerfile -t maswag/arith_homfa:trixie .
```

## Usage examples

Open a shell in the image:

```sh
docker run --rm -it maswag/arith_homfa:noble
```

Use `maswag/arith_homfa:trixie` in the same commands to run the Debian 13 image.

Run the command-line utility directly:

```sh
docker run --rm maswag/arith_homfa:noble ahomfa_util --help
```

Run one of the bundled demo scripts:

```sh
docker run --rm -it maswag/arith_homfa:noble ./examples/run_bg.sh --formula 1 --mode fast
```
