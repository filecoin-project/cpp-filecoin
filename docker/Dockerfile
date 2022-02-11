FROM ubuntu:20.04 AS base
RUN rm -f /etc/apt/apt.conf.d/docker-clean
ARG DEBIAN_FRONTEND=noninteractive
RUN --mount=type=cache,target=/var/cache/apt,sharing=shared apt-get update && apt-get install -y --no-install-recommends hwloc ocl-icd-* curl \
    && rm -rf /var/lib/apt/lists/*
RUN groupadd --gid 1000 fuhon && useradd --uid 1000 --gid fuhon fuhon

FROM base AS build
RUN --mount=type=cache,target=/var/cache/apt,sharing=shared apt-get update && apt-get install -y --no-install-recommends git curl rsync make ninja-build clang-9 jq python3-pip python-setuptools pkg-config opencl-headers libhwloc-dev zlib1g-dev libbz2-dev liblzma-dev libssl-dev libxml2-dev \
    && rm -rf /var/lib/apt/lists/*
RUN pip3 install scikit-build cmake requests gitpython pyyaml
RUN curl -sL https://golang.org/dl/go1.17.3.linux-amd64.tar.gz | tar -xz -C /usr/local
ENV PATH="$PATH:/usr/local/go/bin"
COPY . /tmp/cpp-filecoin
RUN git -C /tmp/cpp-filecoin submodule update --init --recursive
RUN --mount=type=cache,target=/tmp/.hunter/_Base/Cache CC=clang-9 CXX=clang++-9 cmake /tmp/cpp-filecoin -B /tmp/build -G Ninja -D CMAKE_BUILD_TYPE=Release -D TESTING=OFF
RUN --mount=type=cache,target=/tmp/.hunter/_Base/Cache CC=clang-9 cmake --build /tmp/build --target fuhon-node fuhon-miner

FROM base AS fuhon-miner
RUN --mount=type=cache,target=/var/cache/apt,sharing=shared apt-get update && apt-get install -y --no-install-recommends libarchive13 libssl1.1 \
    && rm -rf /var/lib/apt/lists/*
# cmake built libarchive has API v17, but provided version is v13 (https://github.com/libarchive/libarchive/issues/1236)
RUN ln -s /lib/x86_64-linux-gnu/libarchive.so.13 /lib/x86_64-linux-gnu/libarchive.so.17
COPY --from=build /tmp/build/bin/fuhon-miner /usr/local/bin
COPY miner_entrypoint.sh /
RUN chmod +x /miner_entrypoint.sh
USER fuhon
RUN mkdir /tmp/fuhon
WORKDIR /tmp/fuhon
ENTRYPOINT ["/miner_entrypoint.sh"]
CMD ["fuhon-miner", "--repo", "fuhon-node-repo", "--miner-repo", "fuhon-miner-repo"]
HEALTHCHECK --interval=5s --timeout=10s CMD curl http://127.0.0.1:1234/health || exit 1

FROM base AS fuhon-node
COPY --from=build /tmp/build/bin/fuhon-node /usr/local/bin
COPY node_entrypoint.sh /
RUN chmod +x /node_entrypoint.sh
EXPOSE 1234 2000
USER fuhon
RUN mkdir /tmp/fuhon
WORKDIR /tmp/fuhon
ENTRYPOINT ["/node_entrypoint.sh"]
CMD ["fuhon-node", "--config", "fuhon-node.cfg", "--genesis" ,"genesis.car", "--repo", "fuhon-node-repo"]
HEALTHCHECK --interval=5s --timeout=10s CMD curl http://127.0.0.1:1234/health || exit 1
