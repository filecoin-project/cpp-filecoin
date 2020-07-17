#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(resources
        active_resources.cpp
        )

target_link_libraries(resources
        Boost::boost
        )
