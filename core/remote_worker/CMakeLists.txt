#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(remote_worker_main
        main.cpp
        )
target_link_libraries(remote_worker_main
        fetch_handler

        worker
        )
