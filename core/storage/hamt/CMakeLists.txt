#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_hamt
    hamt.cpp
    )
target_link_libraries(filecoin_hamt
    filecoin_blob
    filecoin_cbor
    filecoin_cid
    filecoin_murmur
    filecoin_outcome
    )
