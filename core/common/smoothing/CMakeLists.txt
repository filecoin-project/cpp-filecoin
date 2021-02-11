# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(alpha_beta_filter
    alpha_beta_filter.cpp
    )

target_link_libraries(alpha_beta_filter
    Boost::boost
    )
