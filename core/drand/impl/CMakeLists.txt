#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(drand_marshaller
    parser.cpp
    )
target_link_libraries(drand_marshaller
    drand_proto
    )

add_library(drand_client
    client.cpp
    )
target_link_libraries(drand_client
    Boost::boost
    drand_grpc_proto
    drand_marshaller
    )
