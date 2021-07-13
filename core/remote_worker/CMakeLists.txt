#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(remote_worker_main
    main.cpp
    )
target_link_libraries(remote_worker_main
    Boost::program_options
    filecoin_config
    remote_index
    remote_return
    fetch_handler
    worker
    proof_param_provider
    )
