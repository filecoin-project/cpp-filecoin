#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(runtime_test
    runtime_test.cpp
    )
target_link_libraries(runtime_test
    runtime
    hamt
    )
