# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

#test ticket
addtest(filecoin_ticket_test
    ticket_test.cpp
    )
target_link_libraries(filecoin_ticket_test
    filecoin_ticket_generator
    filecoin_ticket_printer
    )

# test epost ticket and proof
addtest(filecoin_epost_ticket_test
    epost_ticket_test.cpp
    )
target_link_libraries(filecoin_epost_ticket_test
    filecoin_ticket_generator
    filecoin_ticket_printer
    )

#test ticket encode/decode
addtest(filecoin_ticket_codec_test
    ticket_codec_test.cpp
    )
target_link_libraries(filecoin_ticket_codec_test
    filecoin_ticket_generator
    filecoin_ticket_printer
    )

#test epost ticket and encode/decode
addtest(filecoin_epost_ticket_codec_test
    epost_ticket_codec_test.cpp
    )
target_link_libraries(filecoin_epost_ticket_codec_test
    filecoin_ticket_generator
    filecoin_ticket_printer
    )

#test epost proof encode/decode
addtest(filecoin_epost_proof_codec_test
    epost_proof_codec_test.cpp
    )
target_link_libraries(filecoin_epost_proof_codec_test
    filecoin_ticket_generator
    filecoin_ticket_printer
    )
