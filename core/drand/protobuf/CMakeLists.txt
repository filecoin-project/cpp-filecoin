#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_proto_library(drand_proto
    api.proto
    )
add_grpc_proto_library(drand_grpc_proto
    api.proto
    )
target_link_libraries(drand_grpc_proto
    drand_proto
    )
