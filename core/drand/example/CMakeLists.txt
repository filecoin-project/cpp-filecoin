#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(drand_example
    drand_example.cpp
    )
target_link_libraries(drand_example
    drand_client
    )
