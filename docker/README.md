# Docker support for ArithHomFA

Prebuilt ArithHomFA images are published on Docker Hub as `maswag/arith_homfa:noble` and `maswag/arith_homfa:trixie`. Normal users can run those images directly and do not need to build them locally.

This directory contains Dockerfiles for the supported ArithHomFA operating systems: Ubuntu 24.04 LTS and Debian 13 (trixie). Ubuntu 22.04 is not supported.

## Files

- `docker/build_images.sh` - Script to build all supported Docker images
- `docker/ubuntu24.04/Dockerfile`
- `docker/debian-trixie/Dockerfile`

Both images:

- start from the official `ubuntu:noble` or `debian:trixie` image,
- install build dependencies with `apt`,
- build and install Microsoft SEAL 4.3.2 by default,
- build and install Spot from source,
- fetch the pinned upstream repository commit into the image, and
- build the repository with CMake.

The Dockerfiles build the pinned upstream commit
`8fb07f1b2b06c847f6a21ce79ffb26a425b4b119` for reproducibility.

## Published images

Pull the published images explicitly:

```sh
docker pull maswag/arith_homfa:noble
docker pull maswag/arith_homfa:trixie
```

You can also skip `docker pull`; `docker run` pulls the image automatically when it is missing locally.

Open a shell in the Ubuntu 24.04 image:

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

## Optional local build commands

Build images locally only when you need to modify the Dockerfiles or test a different dependency version.

Quick build using the provided script:

```sh
./docker/build_images.sh
```

Manual build:

```sh
docker build -f docker/ubuntu24.04/Dockerfile -t maswag/arith_homfa:noble .
docker build -f docker/debian-trixie/Dockerfile -t maswag/arith_homfa:trixie .
```

## SEAL version

The published images use Microsoft SEAL `4.3.2`. The Dockerfiles build Microsoft SEAL from source, so local custom images can use a different compatible SEAL version.

To build both supported images locally with a different compatible SEAL version, set
`SEAL_VERSION` when running the build script:

```sh
SEAL_VERSION=4.2.0 ./docker/build_images.sh
```

For a manual build, pass the same value as a Docker build argument:

```sh
docker build \
  -f docker/ubuntu24.04/Dockerfile \
  --build-arg SEAL_VERSION=4.2.0 \
  -t maswag/arith_homfa:noble-seal4.2.0 \
  .

docker build \
  -f docker/debian-trixie/Dockerfile \
  --build-arg SEAL_VERSION=4.2.0 \
  -t maswag/arith_homfa:trixie-seal4.2.0 \
  .
```

## Spot version

The published images use Spot `2.15.1`. The Dockerfiles build Spot from source, so local custom images can use a different Spot version.

To build both supported images locally with a different Spot version, set
`SPOT_VERSION` when running the build script:

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
