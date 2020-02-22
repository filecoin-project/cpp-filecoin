#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(tinycbor
    tinycbor/src/cborerrorstrings.c
    tinycbor/src/cborencoder.c
    tinycbor/src/cborencoder_close_container_checked.c
    tinycbor/src/cborparser.c
    tinycbor/src/cborparser_dup_string.c
    tinycbor/src/cborpretty.c
    tinycbor/src/cborpretty_stdio.c
    tinycbor/src/cborvalidation.c
    )
disable_clang_tidy(tinycbor)

# Copy headers for core code to imitate install step
set(TINY_CBOR_HEADERS
    cbor.h
    tinycbor-version.h
    )
foreach(header ${TINY_CBOR_HEADERS})
    configure_file(tinycbor/src/${header}
        include/tinycbor/${header}
        COPYONLY
        )
endforeach()

set(TINY_CBOR_INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}/deps/include
    PARENT_SCOPE
    )
