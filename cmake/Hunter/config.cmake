hunter_config(
    libarchive
    URL https://github.com/soramitsu/fuhon-libarchive/archive/hunter-v3.4.3.tar.gz
    SHA1 0996fd781195df120744164ba5e0033a14c79e06
    CMAKE_ARGS ENABLE_INSTALL=ON
)

hunter_config(
    CURL
    VERSION 7.60.0-p2
    CMAKE_ARGS "HTTP_ONLY=ON"
)

hunter_config(
    spdlog
    VERSION 1.4.2-58e6890-p0
)

hunter_config(libp2p
    URL https://github.com/soramitsu/cpp-libp2p/archive/7bd8ef1c7a4341f2045eaa04062ced6736cbf0bf.tar.gz
    SHA1 ce911568445514ef1873c986f802f8eb971ff789
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )
