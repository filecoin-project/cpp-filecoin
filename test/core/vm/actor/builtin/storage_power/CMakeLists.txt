#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(storage_power_actor_test
    storage_power_actor_test.cpp
    )
target_link_libraries(storage_power_actor_test
    storage_power_actor
    hexutil
    )
