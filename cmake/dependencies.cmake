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
endif()

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
if (HUNTER_ENABLED)
  find_package(leveldb CONFIG REQUIRED)
else()
  find_package(leveldb REQUIRED)
  include_directories(${LEVELDB_INCLUDE_DIRS})
endif()

# https://github.com/soramitsu/libp2p
hunter_add_package(libp2p)
find_package(libp2p CONFIG REQUIRED)

hunter_add_package(soralog)
find_package(soralog CONFIG REQUIRED)

hunter_add_package(yaml-cpp)
find_package(yaml-cpp CONFIG REQUIRED)

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/cppcodec.html
hunter_add_package(cppcodec)
if (HUNTER_ENABLED)
  find_package(cppcodec CONFIG REQUIRED)
else()
  find_package(cppcodec REQUIRED)
  include_directories(${CPPCODEC_INCLUDE_DIRS})
endif()

# http://rapidjson.org
hunter_add_package(RapidJSON)
find_package(RapidJSON CONFIG REQUIRED)

# https://thalhammer.it/projects/jwt_cpp
hunter_add_package(jwt-cpp)
find_package(jwt-cpp CONFIG REQUIRED)

hunter_add_package(libarchive)
find_package(libarchive CONFIG REQUIRED)

hunter_add_package(prometheus-cpp)
find_package(prometheus-cpp CONFIG REQUIRED)

# Add filecoin_ffi target if building without git submodules
if (NOT BUILD_INTERNAL_DEPS)
  find_package(filecoin_ffi REQUIRED)
  include_directories(${FILECOIN_FFI_INCLUDE_DIRS})
endif()
