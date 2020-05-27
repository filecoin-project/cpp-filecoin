#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

add_library(block_producer
    block_producer.cpp
    )
target_link_libraries(block_producer
    bls_provider
    interpreter
    tipset
    weight_calculator
    )
