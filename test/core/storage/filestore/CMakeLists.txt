# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_filestore_test
    filesystem/filesystem_file_test.cpp
    filesystem/filesystem_filestore_test.cpp
    )
target_link_libraries(filecoin_filestore_test
    filecoin_base_fs_test
    filecoin_filestore
    )
