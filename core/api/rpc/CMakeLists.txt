#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(rpc
    ws.cpp
    wsc.cpp
    web_socket_client_error.cpp
    )
target_link_libraries(rpc
    api
    json
    tipset
    )
