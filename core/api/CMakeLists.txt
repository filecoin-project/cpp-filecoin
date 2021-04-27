#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(full_node)

add_library(rpc
    rpc/json_errors.cpp
    rpc/ws.cpp
    rpc/wsc.cpp
    rpc/web_socket_client_error.cpp
    )
target_link_libraries(rpc
    api
    json
    tipset
    )
