#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(fslock_test
        fslock_test.cpp
        )

target_link_libraries(fslock_test
        fslock
        base_fs_test
        filestore
        )
