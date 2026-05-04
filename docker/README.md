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
- build and install Spot from source,
- fetch the pinned upstream repository commit into the image, and
- build the repository with CMake/Ninja.

The Dockerfiles build the pinned upstream commit
`8fb07f1b2b06c847f6a21ce79ffb26a425b4b119` for reproducibility.

## Build commands

Quick build using the provided script:

```sh
./docker/build_images.sh
```

Manual build:

```sh
docker build -f docker/ubuntu24.04/Dockerfile -t maswag/arith_homfa:noble .
docker build -f docker/debian-trixie/Dockerfile -t maswag/arith_homfa:trixie .
```

## Spot version

Both Dockerfiles build Spot from source. The default Spot version is `2.15.1`.

To build both supported images with a different Spot version, set `SPOT_VERSION`
when running the build script:

```sh
SPOT_VERSION=2.10.4 ./docker/build_images.sh
```

For a manual build, pass the same value as a Docker build argument:

```sh
docker build \
  -f docker/ubuntu24.04/Dockerfile \
  --build-arg SPOT_VERSION=2.10.4 \
  -t maswag/arith_homfa:noble-spot2.10.4 \
  .

docker build \
  -f docker/debian-trixie/Dockerfile \
  --build-arg SPOT_VERSION=2.10.4 \
  -t maswag/arith_homfa:trixie-spot2.10.4 \
  .
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
