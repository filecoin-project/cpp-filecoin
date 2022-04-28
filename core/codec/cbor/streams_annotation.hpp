/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdexcept>

#define CBOR_ENCODE(type, var)                                            \
  template <class Stream,                                                 \
            typename = std::enable_if_t<                                  \
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>> \
  Stream &operator<<(Stream &&s,                                          \
                     const type &var)  // NOLINT(bugprone-macro-parentheses)

#define CBOR_DECODE(type, var)                                            \
  template <class Stream,                                                 \
            typename = std::enable_if_t<                                  \
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>> \
  Stream &operator>>(Stream &&s,                                          \
                     type &var)  // NOLINT(bugprone-macro-parentheses)

#define CBOR2_DECODE(...)                          \
  /* NOLINTNEXTLINE(bugprone-macro-parentheses) */ \
  fc::codec::cbor::CborDecodeStream &operator>>(   \
      fc::codec::cbor::CborDecodeStream &s, __VA_ARGS__ &v)
#define CBOR2_ENCODE(...)                        \
  fc::codec::cbor::CborEncodeStream &operator<<( \
      fc::codec::cbor::CborEncodeStream &s, const __VA_ARGS__ &v)
#define CBOR2_DECODE_ENCODE(...) \
  CBOR2_DECODE(__VA_ARGS__);     \
  CBOR2_ENCODE(__VA_ARGS__);

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_1(op, m) op t.m
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_2(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_1(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_3(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_2(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_4(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_3(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_5(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_4(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_6(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_5(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_7(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_6(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_8(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_7(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_9(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_8(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_10(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_9(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_11(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_10(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_12(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_11(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_13(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_12(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_14(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_13(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_15(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_14(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_16(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_15(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_17(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_16(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_18(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_17(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_19(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_18(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_20(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_19(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_21(op, m, ...) \
  _CBOR_TUPLE_1(op, m) _CBOR_TUPLE_20(op, __VA_ARGS__)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE_V(_1,  \
                      _2,  \
                      _3,  \
                      _4,  \
                      _5,  \
                      _6,  \
                      _7,  \
                      _8,  \
                      _9,  \
                      _10, \
                      _11, \
                      _12, \
                      _13, \
                      _14, \
                      _15, \
                      _16, \
                      _17, \
                      _18, \
                      _19, \
                      _20, \
                      _21, \
                      f,   \
                      ...) \
  f
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CBOR_TUPLE(op, ...)    \
  _CBOR_TUPLE_V(__VA_ARGS__,    \
                _CBOR_TUPLE_21, \
                _CBOR_TUPLE_20, \
                _CBOR_TUPLE_19, \
                _CBOR_TUPLE_18, \
                _CBOR_TUPLE_17, \
                _CBOR_TUPLE_16, \
                _CBOR_TUPLE_15, \
                _CBOR_TUPLE_14, \
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

#define CBOR_ENCODE_TUPLE(T, ...)                 \
  CBOR_ENCODE(T, t) {                             \
    auto l{s.list()};                             \
    return s << (l _CBOR_TUPLE(<<, __VA_ARGS__)); \
  }

#define CBOR_TUPLE(T, ...)          \
  CBOR_ENCODE_TUPLE(T, __VA_ARGS__) \
  CBOR_DECODE(T, t) {               \
    auto l{s.list()};               \
    l _CBOR_TUPLE(>>, __VA_ARGS__); \
    return s;                       \
  }

#define CBOR_TUPLE_0(T) \
  CBOR_ENCODE(T, t) {   \
    s << s.list();      \
    return s;           \
  }                     \
  CBOR_DECODE(T, t) {   \
    s.list();           \
    return s;           \
  }

#define CBOR_NON(T)                                                           \
  CBOR_ENCODE(T, t) {                                                         \
    throw std::runtime_error("CBOR encode must not be called for this type"); \
  }                                                                           \
  CBOR_DECODE(T, t) {                                                         \
    throw std::runtime_error("CBOR decode must not be called for this type"); \
  }

namespace fc::codec::cbor {
  class CborDecodeStream;
  class CborEncodeStream;
}  // namespace fc::codec::cbor
