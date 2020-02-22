#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_interpreter
    impl/interpreter_impl.cpp
    )
target_link_libraries(filecoin_interpreter
    filecoin_amt
    )
