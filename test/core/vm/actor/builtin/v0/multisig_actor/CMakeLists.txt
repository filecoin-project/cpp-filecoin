#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(multisig_actor_test
    multisig_actor_test.cpp
    )
target_link_libraries(multisig_actor_test
    multisig_actor
    )
