#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(data_transfer
    dt.cpp
    )
target_link_libraries(data_transfer
    cbor_stream
    graphsync
    stream_open_queue
    )
