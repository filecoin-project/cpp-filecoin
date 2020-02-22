#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_hexutil
    hexutil.hpp
    hexutil.cpp
    )
target_link_libraries(filecoin_hexutil
    Boost::boost
    filecoin_outcome
    )

add_library(filecoin_blob
    blob.hpp
    blob.cpp
    )
target_link_libraries(filecoin_blob
    filecoin_hexutil
    )

add_library(filecoin_outcome INTERFACE)
target_link_libraries(filecoin_outcome INTERFACE
    Boost::boost
    p2p::p2p
    )

add_library(filecoin_buffer
    buffer.hpp
    buffer.cpp
    buffer_back_insert_iterator.cpp
    )
target_link_libraries(filecoin_buffer
    filecoin_hexutil
    )

add_library(filecoin_logger
    logger.cpp
    )
target_link_libraries(filecoin_logger
    spdlog::spdlog
    )
