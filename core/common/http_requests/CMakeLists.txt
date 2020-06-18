#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(requests
        impl/request_impl.cpp
        impl/request_factory_impl.cpp
        )

target_link_libraries(requests
        CURL::libcurl
        outcome
        )
