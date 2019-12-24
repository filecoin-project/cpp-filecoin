# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filesystem_repository_test
    filesystem_repository_test.cpp
    )
target_link_libraries(filesystem_repository_test
    filesystem_repository
    base_fs_test
    )
