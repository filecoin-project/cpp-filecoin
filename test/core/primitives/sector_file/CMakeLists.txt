#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(sector_file_test
        sector_file_test.cpp
        )

target_link_libraries(sector_file_test
        sector_file
        )
