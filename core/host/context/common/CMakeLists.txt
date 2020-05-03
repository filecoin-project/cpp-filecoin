#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(host_common_context
    common_context_impl.cpp
    )
target_link_libraries(host_common_context
    Boost::boost
    )
