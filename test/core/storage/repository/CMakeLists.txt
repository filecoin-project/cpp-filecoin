# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_filesystem_repository_test
    filesystem_repository_test.cpp
    )
target_link_libraries(filecoin_filesystem_repository_test
    filecoin_filesystem_repository
    filecoin_base_fs_test
    )
