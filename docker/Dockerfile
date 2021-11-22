FROM ubuntu:20.04 AS base
RUN rm -f /etc/apt/apt.conf.d/docker-clean
RUN --mount=type=cache,target=/var/cache/apt apt-get update

FROM base AS build
RUN --mount=type=cache,target=/var/cache/apt DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends git curl rsync make ninja-build clang-9 jq python3-pip python-setuptools pkg-config ocl-icd-* opencl-headers libhwloc-dev
RUN pip3 install scikit-build cmake requests gitpython pyyaml
RUN curl -sL https://golang.org/dl/go1.17.3.linux-amd64.tar.gz | tar -xz -C /usr/local
ENV PATH="$PATH:/usr/local/go/bin"
COPY . /root/cpp-filecoin
RUN --mount=type=cache,target=/root/.hunter/_Base/Cache CC=clang-9 CXX=clang++-9 cmake /root/cpp-filecoin -B /root/build -G Ninja -D CMAKE_BUILD_TYPE=Release -D TESTING=OFF
RUN --mount=type=cache,target=/root/.hunter/_Base/Cache CC=clang-9 cmake --build /root/build --target fuhon-node

FROM base
RUN --mount=type=cache,target=/var/cache/apt apt-get install -y --no-install-recommends hwloc ocl-icd-*
COPY core/node/main/mainnet.car /root/.
COPY --from=build /root/build/bin/fuhon-node /root/.
EXPOSE 1234 2000
