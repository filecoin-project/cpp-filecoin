#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_cron_actor
    cron_actor.cpp
    )
target_link_libraries(filecoin_cron_actor
    filecoin_actor
    filecoin_outcome
    )
