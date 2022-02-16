#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(cli_example main.cpp)
target_link_libraries(cli_example
    cli
    rpc
    car
    file
    memory_indexed_car
    unixfs
    )
