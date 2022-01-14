## Prerequisites

[Docker BuildKit](https://docs.docker.com/develop/develop-images/build_enhancements/#to-enable-buildkit-builds)

`DOCKER_BUILDKIT=1` environment variable must be set to enable BuildKit. 

## Build

To build docker image from the current context. It is advised to ignore binaries and large files from docker build context.

    docker build .. -f Dockerfile

There are 2 targets `fuhon-node` and `fuhon-miner` declared in Dockerfile. To build a specific target use `-t` option.