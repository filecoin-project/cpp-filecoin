#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_cron_actor_test
    cron_actor_test.cpp
    )
target_link_libraries(filecoin_cron_actor_test
    filecoin_cron_actor
    )
