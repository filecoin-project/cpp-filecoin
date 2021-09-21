#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(libp2p)
add_subdirectory(smoothing)
add_subdirectory(uri_parser)

add_library(hexutil
    hexutil.hpp
    hexutil.cpp
    )
target_link_libraries(hexutil
    Boost::boost
    outcome
    )

add_library(blob
    blob.hpp
    blob.cpp
    )
target_link_libraries(blob
    hexutil
    )

add_library(outcome
    error_text.cpp
    outcome2.cpp
    )
target_link_libraries(outcome
    Boost::boost
    p2p::p2p
    )

add_library(buffer
    buffer.hpp
    buffer.cpp
    buffer_back_insert_iterator.cpp
    )
target_link_libraries(buffer
    hexutil
    )

add_library(logger
    logger.cpp
    )
target_link_libraries(logger
    spdlog::spdlog
    )

add_library(tarutil
    tarutil.cpp
    )

target_link_libraries(tarutil
    outcome
    logger
    libarchive::archive
    )

add_library(file
    file.cpp
    )
target_link_libraries(file
    Boost::iostreams
    buffer
    outcome
    )

add_library(jwt INTERFACE)

target_link_libraries(jwt INTERFACE
    OpenSSL::Crypto
    )
