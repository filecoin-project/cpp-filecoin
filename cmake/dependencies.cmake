# hunter dependencies
# https://docs.hunter.sh/en/latest/packages/

# Append local modules path if Hunter is not enabled
if (NOT HUNTER_ENABLED)
  list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
endif()

if (TESTING)
  # https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
  hunter_add_package(GTest)
  find_package(GTest CONFIG REQUIRED)
  find_package(GMock CONFIG REQUIRED)
endif()

# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost COMPONENTS date_time filesystem random system)
find_package(Boost CONFIG REQUIRED date_time filesystem random system)

# https://docs.hunter.sh/en/latest/packages/pkg/Microsoft.GSL.html
hunter_add_package(Microsoft.GSL)

# https://www.openssl.org/
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

# https://developers.google.com/protocol-buffers/
hunter_add_package(Protobuf)
find_package(Protobuf REQUIRED)

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
find_package(cppcodec REQUIRED)
