#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_fslock_test
        fslock_test.cpp
        )

target_link_libraries(filecoin_fslock_test
        filecoin_fslock
        filecoin_base_fs_test
        filecoin_filestore
        )
