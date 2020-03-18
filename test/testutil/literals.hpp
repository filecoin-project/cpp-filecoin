/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_LITERALS_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_LITERALS_HPP

#include <libp2p/multi/content_identifier_codec.hpp>

#include "common/blob.hpp"
#include "common/hexutil.hpp"
#include "primitives/cid/cid.hpp"

inline std::vector<uint8_t> operator""_unhex(const char *c, size_t s) {
  return fc::common::unhex(std::string_view(c, s)).value();
}

inline fc::common::Hash256 operator""_hash256(const char *c, size_t s) {
  return fc::common::Hash256::fromHex(std::string_view(c, s)).value();
}

inline fc::common::Blob<20> operator""_blob20(const char *c, size_t s) {
  return fc::common::Blob<20>::fromHex(std::string_view(c, s)).value();
}

inline fc::common::Blob<32> operator""_blob32(const char *c, size_t s) {
  return fc::common::Blob<32>::fromHex(std::string_view(c, s)).value();
}

inline fc::common::Blob<48> operator""_blob48(const char *c, size_t s) {
  return fc::common::Blob<48>::fromHex(std::string_view(c, s)).value();
}

inline fc::common::Blob<96> operator""_blob96(const char *c, size_t s) {
  return fc::common::Blob<96>::fromHex(std::string_view(c, s)).value();
}

inline auto operator""_cid(const char *c, size_t s) {
  auto cid = libp2p::multi::ContentIdentifierCodec::decode(
                 fc::common::unhex(std::string_view(c, s)).value())
                 .value();
  return fc::CID{std::move(cid)};
}

#endif  // CPP_FILECOIN_TEST_TESTUTIL_LITERALS_HPP
