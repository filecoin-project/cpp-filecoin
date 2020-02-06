#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_custom_target(adt
    DEPENDS
    array
    )

add_library(array
    impl/array.cpp
    )
target_link_libraries(array
    amt
    )
