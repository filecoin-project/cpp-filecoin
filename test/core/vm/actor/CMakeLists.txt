#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(builtin)

addtest(filecoin_actor_test
    actor_test.cpp
    )
target_link_libraries(filecoin_actor_test
    filecoin_actor
    filecoin_hexutil
    )

addtest(filecoin_invoker_test
    invoker_test.cpp
    )
target_link_libraries(filecoin_invoker_test
    filecoin_actor
    filecoin_hamt
    )
