# hunter dependencies
# https://docs.hunter.sh/en/latest/packages/

# https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
find_package(GMock CONFIG REQUIRED)

hunter_add_package(libarchive)
find_package(libarchive CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost COMPONENTS date_time filesystem iostreams random program_options thread)
find_package(Boost CONFIG REQUIRED date_time filesystem iostreams random program_options thread)

# https://docs.hunter.sh/en/latest/packages/pkg/Microsoft.GSL.html
hunter_add_package(Microsoft.GSL)
find_package(Microsoft.GSL CONFIG REQUIRED)

# https://www.openssl.org/
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

# https://hunter.readthedocs.io/en/latest/packages/pkg/CURL.html#pkg-curl
hunter_add_package(CURL)
find_package(CURL CONFIG REQUIRED)

# https://developers.google.com/protocol-buffers/
hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)

# https://github.com/hunter-packages/grpc
hunter_add_package(gRPC)
find_package(gRPC CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/spdlog.html
hunter_add_package(spdlog)
find_package(spdlog CONFIG REQUIRED)

# https://github.com/masterjedy/hat-trie
hunter_add_package(tsl_hat_trie)
find_package(tsl_hat_trie CONFIG REQUIRED)

# https://github.com/masterjedy/di
hunter_add_package(Boost.DI)
find_package(Boost.DI CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/leveldb.html
hunter_add_package(leveldb)
find_package(leveldb CONFIG REQUIRED)

# https://github.com/soramitsu/libp2p
hunter_add_package(libp2p)
find_package(libp2p CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/cppcodec.html
hunter_add_package(cppcodec)
find_package(cppcodec CONFIG REQUIRED)

# http://rapidjson.org
hunter_add_package(RapidJSON)
find_package(RapidJSON CONFIG REQUIRED)

# https://github.com/soramitsu/libp2p-sqlite-modern-cpp/tree/hunter
hunter_add_package(SQLiteModernCpp)
find_package(SQLiteModernCpp CONFIG REQUIRED)
