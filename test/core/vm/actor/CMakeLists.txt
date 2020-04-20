#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(builtin)

addtest(invoker_test
    invoker_test.cpp
    )
target_link_libraries(invoker_test
    actor
    hamt
    )
