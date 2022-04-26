hunter_config(
    libarchive
    URL https://github.com/Markuu-s/fuhon-libarchive/archive/fc8661c5619505ec252196e7e1b9436edf91cd58.tar.gz
    SHA1 5e21e7e10a36dfeafbe5fa665254faf83ba56ee3
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
    URL https://github.com/soramitsu/cpp-libp2p/archive/3587bdcef771d6ca93b3f00bb26a6603653bdb5f.tar.gz
    SHA1 45ae022b61dac3fb5396cfa1eb89cbc41e9a3b43
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )

hunter_config(jwt-cpp
    URL https://github.com/soramitsu/fuhon-jwt-cpp/archive/ac0424b115721e4066d2fb99f72ba0cd58759882.tar.gz
    SHA1 87ad78edb0412e1dcb7ef3de28c0a818b3ab0915
    CMAKE_ARGS JWT_BUILD_EXAMPLES=OFF
    )
