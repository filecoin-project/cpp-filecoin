#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(actor_test
    actor_test.cpp
    )
target_link_libraries(actor_test
    actor
    cbor
    hexutil
    )
