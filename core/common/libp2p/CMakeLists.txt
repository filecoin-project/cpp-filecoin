#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(cbor_stream
    cbor_buffering.cpp
    cbor_stream.cpp
    )
target_link_libraries(cbor_stream
    cbor
    )

add_library(stream_open_queue
    stream_open_queue.cpp
    )
target_link_libraries(stream_open_queue
    p2p::p2p
    )
