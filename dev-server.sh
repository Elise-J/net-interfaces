#!/bin/bash
CONTROL_LIBRARIES_BRANCH=v7.0.0
REMOTE_SSH_PORT=4420

IMAGE_NAME=aica-technology/elise-j/net-interfaces
IMAGE_STAGE=source-dependencies

BUILD_FLAGS=()

HELP_MESSAGE="Usage: ./dev-server.sh [-r] [-v]

Build a Docker container for remote development.
Options:
  -r, --rebuild            Rebuild the image with no cache.
  -v, --verbose            Show all the output of the Docker
                           build process.
  -h, --help               Show this help message."

while [ "$#" -gt 0 ]; do
  case "$1" in
    -r|--rebuild) BUILD_FLAGS+=(--no-cache); shift 1;;
    -v|--verbose) BUILD_FLAGS+=(--progress=plain); shift 1;;
    -h|--help) echo "${HELP_MESSAGE}"; exit 0;;
    *) echo "Unknown option: $1" >&2; echo "${HELP_MESSAGE}"; exit 1;;
  esac
done

BUILD_FLAGS+=(--build-arg CONTROL_LIBRARIES_BRANCH="${CONTROL_LIBRARIES_BRANCH}")
BUILD_FLAGS+=(-t "${IMAGE_NAME}:${IMAGE_STAGE}")
BUILD_FLAGS+=(--target "${IMAGE_STAGE}")

docker pull ghcr.io/aica-technology/ros-control-libraries:noetic || exit 1
# pull ghcr.io/aica-technology/control-libraries/development-dependencies || exit 1
DOCKER_BUILDKIT=1 docker build "${BUILD_FLAGS[@]}" . || exit 1

aica-docker server "${IMAGE_NAME}:${IMAGE_STAGE}" -u developer -p "${REMOTE_SSH_PORT}"
