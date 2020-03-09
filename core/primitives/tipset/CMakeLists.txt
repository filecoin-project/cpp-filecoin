# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

filecoin_add_library(filecoin_tipset
    tipset.cpp
    tipset_key.cpp
    )
target_link_libraries(filecoin_tipset
    filecoin_address
    filecoin_block
    filecoin_buffer
    filecoin_cbor
    filecoin_cid
    filecoin_logger
    filecoin_tickets
    )
