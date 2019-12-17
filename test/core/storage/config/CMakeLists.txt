# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(config_test
    config_test.cpp
    )
target_link_libraries(config_test
    base_fs_test
    config
    )
