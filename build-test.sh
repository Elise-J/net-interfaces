#!/bin/bash
CONTROL_LIBRARIES_BRANCH=v6.0.0

IMAGE_NAME=aica-technology/elise-j/net-interfaces
IMAGE_STAGE=build-test

BUILD_FLAGS=()

HELP_MESSAGE="Usage: ./build.sh [-r] [-v]

Build a Docker container for running unittests.
Options:
  -r, --rebuild            Rebuild the image with no cache.
  -v, --verbose            Show all the output of the Docker
                           build process.
  -h, --help               Show this help message."

while [ "$#" -gt 0 ]; do
  case "$1" in
    -r|--rebuild) BUILD_FLAGS+=(--no-cache); shift 1;;
    -t|--test) IMAGE_STAGE=build-test; shift 1;;
    -v|--verbose) BUILD_FLAGS+=(--progress=plain); shift 1;;
    -h|--help) echo "${HELP_MESSAGE}"; exit 0;;
    *) echo "Unknown option: $1" >&2; echo "${HELP_MESSAGE}"; exit 1;;
  esac
done

BUILD_FLAGS+=(--build-arg CONTROL_LIBRARIES_BRANCH="${CONTROL_LIBRARIES_BRANCH}")
BUILD_FLAGS+=(-t "${IMAGE_NAME}:${IMAGE_STAGE}")
BUILD_FLAGS+=(--target "${IMAGE_STAGE}")

docker pull ghcr.io/aica-technology/control-libraries/development-dependencies || exit 1
DOCKER_BUILDKIT=1 docker build "${BUILD_FLAGS[@]}" .
