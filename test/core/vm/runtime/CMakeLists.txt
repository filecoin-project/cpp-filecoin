#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(filecoin_runtime_test
    runtime_test.cpp
    )
target_link_libraries(filecoin_runtime_test
    filecoin_runtime
    filecoin_hamt
    )
