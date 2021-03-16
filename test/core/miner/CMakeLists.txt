#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(sector_stat_test
        sector_stat_test.cpp
        )
target_link_libraries(sector_stat_test
        sector_stat)

addtest(tipset_cache_test
        tipset_cache_test.cpp
        )
target_link_libraries(tipset_cache_test
        tipset_cache)

addtest(precommit_policy_test
        precommit_policy_test.cpp
        )
target_link_libraries(precommit_policy_test
        precommit_policy)

