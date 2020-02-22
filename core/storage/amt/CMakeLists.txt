#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_amt
    amt.cpp
    )
target_link_libraries(filecoin_amt
    filecoin_cbor
    filecoin_cid
    filecoin_outcome
    filecoin_ipld_block
    )
