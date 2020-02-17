/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STREAMS_ANNOTATION_HPP
#define CPP_FILECOIN_STREAMS_ANNOTATION_HPP

#define CBOR_ENCODE(type, var)                                            \
  template <class Stream,                                                 \
            typename = std::enable_if_t<                                  \
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>> \
  Stream &operator<<(Stream &&s, const type &var)

#define CBOR_DECODE(type, var)                                            \
  template <class Stream,                                                 \
            typename = std::enable_if_t<                                  \
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>> \
  Stream &operator>>(Stream &&s, type &var)

#endif  // CPP_FILECOIN_STREAMS_ANNOTATION_HPP
