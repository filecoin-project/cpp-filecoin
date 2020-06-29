hunter_config(
    Boost
    VERSION 1.72.0-p1
    CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=ON
)

hunter_config(
    GTest
    VERSION 1.8.0-hunter-p11
    CMAKE_ARGS "CMAKE_CXX_FLAGS=-Wno-deprecated-copy"
)

hunter_config(
    CURL
    VERSION 7.60.0-p2
    CMAKE_ARGS "HTTP_ONLY=ON"
)

hunter_config(
    gRPC
    VERSION "1.29.1"
    CMAKE_ARGS "gRPC_BUILD_CSHARP_EXT=OFF"
)

hunter_config(
    spdlog
    URL https://github.com/gabime/spdlog/archive/v1.4.2.zip
    SHA1 4b10e9aa17f7d568e24f464b48358ab46cb6f39c
)

hunter_config(
    tsl_hat_trie
    URL https://github.com/masterjedy/hat-trie/archive/343e0dac54fc8491065e8a059a02db9a2b1248ab.zip
    SHA1 7b0051e9388d629f382752dd6a12aa8918cdc022
)

hunter_config(
    Boost.DI
    URL https://github.com/masterjedy/di/archive/c5287ee710ad90f5286d0cc2b9e49b72d89267a6.zip
    SHA1 802b64a6242be45771f3d4c86257eac0a3c7b289
    CMAKE_ARGS BOOST_DI_OPT_BUILD_TESTS=OFF BOOST_DI_OPT_BUILD_EXAMPLES=OFF # disable building examples and tests
)

hunter_config(
    SQLiteModernCpp
    URL https://github.com/soramitsu/libp2p-sqlite-modern-cpp/archive/fc3b700064cb57ab6b598c9bc7a12b2842f78da2.zip
    SHA1 d913f2a0360892a30bc7cd8820a0475800b47d76
)

hunter_config(libp2p
    URL https://github.com/soramitsu/libp2p/archive/a85364e666e4c4b850b0fa7530b8e1d085d26f3c.zip
    SHA1 990a14421c61b362152718dceb71e4efaa74c1e6
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )
