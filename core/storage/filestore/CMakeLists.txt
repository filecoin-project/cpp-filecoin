# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(filecoin_filestore
    filestore_error.cpp
    impl/filesystem/filesystem_file.cpp
    impl/filesystem/filesystem_filestore.cpp
    )
target_link_libraries(filecoin_filestore
    Boost::filesystem
    filecoin_outcome
    )
