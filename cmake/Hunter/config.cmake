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
    URL https://github.com/soramitsu/cpp-libp2p/archive/7c9d83bf0760a5f653d86ddbb00645414c61d4fc.tar.gz
    SHA1 f4408541230f4953678d1871096786cba2f5fcef
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )
