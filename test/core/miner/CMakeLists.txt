#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(sector_stat_test
        sector_stat_test.cpp
        )
target_link_libraries(sector_stat_test
        sector_stat)
