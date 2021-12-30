FROM ubuntu:20.04 AS base
RUN rm -f /etc/apt/apt.conf.d/docker-clean
RUN --mount=type=cache,target=/var/cache/apt apt-get update
RUN --mount=type=cache,target=/var/cache/apt apt-get install -y --no-install-recommends hwloc ocl-icd-*

FROM base AS build
RUN --mount=type=cache,target=/var/cache/apt DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends git curl rsync make ninja-build clang-9 jq python3-pip python-setuptools pkg-config opencl-headers libhwloc-dev
RUN pip3 install scikit-build cmake requests gitpython pyyaml
RUN curl -sL https://golang.org/dl/go1.17.3.linux-amd64.tar.gz | tar -xz -C /usr/local
ENV PATH="$PATH:/usr/local/go/bin"
COPY . /tmp/cpp-filecoin
RUN --mount=type=cache,target=/tmp/.hunter/_Base/Cache CC=clang-9 CXX=clang++-9 cmake /tmp/cpp-filecoin -B /tmp/build -G Ninja -D CMAKE_BUILD_TYPE=Release -D TESTING=OFF
RUN --mount=type=cache,target=/tmp/.hunter/_Base/Cache CC=clang-9 cmake --build /tmp/build --target fuhon-node

FROM base
WORKDIR /opt/fuhon
COPY --from=build /tmp/build/bin/fuhon-node /usr/local/bin
EXPOSE 1234 2000
COPY docker/entrypoint.sh /
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
