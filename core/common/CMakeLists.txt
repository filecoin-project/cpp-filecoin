#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(hexutil
    hexutil.hpp
    hexutil.cpp
    )
target_link_libraries(hexutil
    Boost::boost
    outcome
    )
filecoin_install(hexutil)

add_library(blob
    blob.hpp
    blob.cpp
    )
target_link_libraries(blob
    hexutil
    )
filecoin_install(blob)

add_library(buffer
    buffer.hpp
    buffer.cpp
    buffer_back_insert_iterator.cpp
    )
target_link_libraries(buffer
    hexutil
    )
filecoin_install(buffer)

add_library(logger
    logger.cpp
    )
target_link_libraries(logger
    spdlog::spdlog
    )
filecoin_install(logger)
