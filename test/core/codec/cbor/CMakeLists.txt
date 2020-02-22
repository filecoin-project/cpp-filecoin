#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_cbor_test
    cbor_test.cpp
    )
target_link_libraries(filecoin_cbor_test
    filecoin_cbor
    filecoin_hexutil
    )
