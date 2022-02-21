#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(cli INTERFACE)
target_link_libraries(cli INTERFACE
    Boost::program_options
    fmt::fmt
    )

add_subdirectory(example)
