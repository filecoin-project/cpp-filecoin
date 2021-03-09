#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(cron_actor_v3_test
    cron_actor_test.cpp
    )
target_link_libraries(cron_actor_v3_test
    cron_actor_v3
    ipfs_datastore_in_memory
    )
