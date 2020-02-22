# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_config_test
    config_test.cpp
    )
target_link_libraries(filecoin_config_test
    filecoin_base_fs_test
    filecoin_config
    )
