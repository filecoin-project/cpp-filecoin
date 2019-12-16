#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(cbor_test
    cbor_test.cpp
    )
target_link_libraries(cbor_test
    cbor
    hexutil
    )
