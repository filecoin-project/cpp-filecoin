#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(cron_actor
    cron_actor.cpp
    )
target_link_libraries(cron_actor
    actor
    outcome
    )
