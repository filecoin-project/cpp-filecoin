/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t *data;
  size_t size;
} Raw;

#ifndef __cplusplus

void free(void *);

#else

#include "codec/cbor/cbor_codec.hpp"

#define GOC_METHOD(name)                    \
  Bytes goc_##name(BytesIn);                \
  extern "C" Raw name(Raw raw) {            \
    return gocRet(goc_##name(gocArg(raw))); \
  }                                         \
  Bytes goc_##name(BytesIn arg)

#define CBOR_METHOD(name)                                   \
  void cbor_##name(CborDecodeStream &, CborEncodeStream &); \
  GOC_METHOD(name) {                                        \
    CborEncodeStream ret;                                   \
    CborDecodeStream _arg{arg};                             \
    cbor_##name(_arg, ret);                                 \
    return ret.data();                                      \
  }                                                         \
  void cbor_##name(CborDecodeStream &arg, CborEncodeStream &ret)

namespace fc {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;

  inline auto gocArg(Raw raw) {
    return gsl::make_span(raw.data, raw.size);
  }

  inline Raw gocRet(BytesIn in) {
    Raw raw{(uint8_t *)malloc(in.size()), (size_t)in.size()};
    memcpy(raw.data, in.data(), raw.size);
    return raw;
  }

  inline Raw cgoArg(BytesIn in) {
    return {(uint8_t *)in.data(), (size_t)in.size()};
  }

  inline Bytes cgoRet(Raw &&raw) {
    const Bytes buffer = copy(gsl::span(raw.data, raw.size));
    free(raw.data);
    return buffer;
  }

  template <auto f>
  inline auto cgoCall(BytesIn arg) {
    return cgoRet(f(cgoArg(arg)));
  }

  template <auto f>
  inline auto cgoCall(const CborEncodeStream &arg) {
    return cgoCall<f>(arg.data());
  }
}  // namespace fc

#endif
