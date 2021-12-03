#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(interpreter
    impl/interpreter_impl.cpp
    impl/cached_interpreter.cpp
    )
target_link_libraries(interpreter
    amt
    block_validator
    message
    runtime
    weight_calculator
    )
