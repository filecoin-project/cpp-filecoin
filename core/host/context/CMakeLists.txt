#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(fuhon_host
    impl/host_context_impl.cpp
    )
target_link_libraries(fuhon_host
    Boost::boost
    )
