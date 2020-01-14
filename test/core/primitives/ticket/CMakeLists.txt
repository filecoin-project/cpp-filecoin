# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

#test ticket
addtest(ticket_test
    ticket_test.cpp
    )
target_link_libraries(ticket_test
    ticket_utils
    )

# test epost ticket and proof
addtest(epost_ticket_test
    epost_ticket_test.cpp
    )
target_link_libraries(epost_ticket_test
    ticket_utils
    )

#test ticket encode/decode
addtest(ticket_codec_test
    ticket_codec_test.cpp
    )
target_link_libraries(ticket_codec_test
    ticket_utils
    )

#test epost ticket and encode/decode
addtest(epost_ticket_codec_test
    epost_ticket_codec_test.cpp
    )
target_link_libraries(epost_ticket_codec_test
    ticket_utils
    )

#test epost proof encode/decode
addtest(epost_proof_codec_test
    epost_proof_codec_test.cpp
    )
target_link_libraries(epost_proof_codec_test
    ticket_utils
    )
