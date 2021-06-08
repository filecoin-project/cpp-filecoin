#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(interpreter_cache_test
    interpreter_cache_test.cpp
    )
target_link_libraries(interpreter_cache_test
    interpreter
    in_memory_storage
    )
