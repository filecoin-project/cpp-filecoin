## Prerequisites

[Docker BuildKit](https://docs.docker.com/develop/develop-images/build_enhancements/#to-enable-buildkit-builds)

## Build

To build docker image from current context. It is advised to ignore binaries and large files from docker build context.

    docker build .. -f Dockerfile