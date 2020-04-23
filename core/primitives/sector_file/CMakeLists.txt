# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(sector_file
        sector_file.cpp)

target_link_libraries(sector_file
        sector
        )
