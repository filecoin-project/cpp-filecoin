#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

add_library(leb128
    leb128_errors.cpp
    )
target_link_libraries(leb128
    Boost::boost
    outcome
    )
