#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(discovery
    impl/discovery_impl.cpp
    )
target_link_libraries(discovery
    address
    cbor
    cbor_stream
    outcome
    )
