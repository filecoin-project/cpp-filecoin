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

#define _CBOR_TUPLE_1(op, m) op t.m
#define _CBOR_TUPLE_2(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_1(op, __VA_ARGS__)
#define _CBOR_TUPLE_3(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_2(op, __VA_ARGS__)
#define _CBOR_TUPLE_4(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_3(op, __VA_ARGS__)
#define _CBOR_TUPLE_5(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_4(op, __VA_ARGS__)
#define _CBOR_TUPLE_6(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_5(op, __VA_ARGS__)
#define _CBOR_TUPLE_7(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_6(op, __VA_ARGS__)
#define _CBOR_TUPLE_8(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_7(op, __VA_ARGS__)
#define _CBOR_TUPLE_9(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_8(op, __VA_ARGS__)
#define _CBOR_TUPLE_10(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_9(op, __VA_ARGS__)
#define _CBOR_TUPLE_11(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_10(op, __VA_ARGS__)
#define _CBOR_TUPLE_12(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_11(op, __VA_ARGS__)
#define _CBOR_TUPLE_13(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_12(op, __VA_ARGS__)
#define _CBOR_TUPLE_V(                                              \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, f, ...) \
  f
#define _CBOR_TUPLE(op, ...)    \
  _CBOR_TUPLE_V(__VA_ARGS__,    \
                _CBOR_TUPLE_13, \
                _CBOR_TUPLE_12, \
                _CBOR_TUPLE_11, \
                _CBOR_TUPLE_10, \
                _CBOR_TUPLE_9,  \
                _CBOR_TUPLE_8,  \
                _CBOR_TUPLE_7,  \
                _CBOR_TUPLE_6,  \
                _CBOR_TUPLE_5,  \
                _CBOR_TUPLE_4,  \
                _CBOR_TUPLE_3,  \
                _CBOR_TUPLE_2,  \
                _CBOR_TUPLE_1)  \
  (op, __VA_ARGS__)

#define CBOR_ENCODE_TUPLE(T, ...)                        \
  CBOR_ENCODE(T, t) {                                    \
    return s << (s.list() _CBOR_TUPLE(<<, __VA_ARGS__)); \
  }

#define CBOR_TUPLE(T, ...)                 \
  CBOR_ENCODE_TUPLE(T, __VA_ARGS__)        \
  CBOR_DECODE(T, t) {                      \
    s.list() _CBOR_TUPLE(>>, __VA_ARGS__); \
    return s;                              \
  }

#endif  // CPP_FILECOIN_STREAMS_ANNOTATION_HPP
