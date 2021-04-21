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
    URL https://github.com/soramitsu/cpp-libp2p/archive/c9fb62437133ddcd5d17c548aa9f0fa5eab3f59d.tar.gz
    SHA1 e7a082e9c98796e15f7293aee0640210d265ce6a
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )
