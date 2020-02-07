/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/json_codec.hpp"
#include "codec/cbor/cbor.hpp"
#include <boost/property_tree/ptree.hpp
#include <boost/property_tree/json_parser.hpp>
#include <boost/iostreams/stream_buffer.hpp>

namespace fc::codec::json {

  namespace {
    // TODO(yuraz): implement correct encode and decode
    // this solution is temporary
    outcome::result<common::Buffer> encodeCid(const CID &cid) {
      OUTCOME_TRY(buffer, cbor::encode(cid));
      return common::Buffer(buffer);
    }

    outcome::result<CID> decodeCid(gsl::span<const uint8_t> data) {
      return cbor::decode<CID>(data);
    }
  }  // namespace

  outcome::result<common::Buffer> encodeCidVector(gsl::span<const CID> span) {
    boost::property_tree::ptree tree;
    std::vector<common::Buffer> cids;
    cids.reserve(span.size());
    for (auto &it : span) {
      OUTCOME_TRY(encoded, encodeCid(it));
      cids.push_back(encoded);
    }
    tree.put("/", cids);

    return common::Buffer{
        std::vector<uint8_t>(tree.data().begin(), tree.data().end())};
  }

  outcome::result<std::vector<CID>> decodeCidVector(
      gsl::span<const uint8_t> data) {
    boost::property_tree::ptree tree;
    typedef boost::iostreams::basic_array_source<uint8_t> Device;
    boost::iostreams::stream_buffer<Device> stream(data);
    
    stream << data;
    try {
      boost::property_tree::read_json(stream, tree);
    } catch (const boost::property_tree::json_parser::json_parser_error &) {
      return JsonCodecError::BAD_JSON;
    }
    if (tree.count("/") == 0) {
      return JsonCodecError::WRONG_CID_ARRAY_FORMAT;
    }

    auto &&array = tree.get_child("/");
    std::vector<CID> cids;
    for (auto &&it : array) {
      auto &&val = it.second.get_value<std::string>();
      std::vector<uint8_t> span{val.begin(), val.end()};
      OUTCOME_TRY(cid, decodeCid(span));
      cids.push_back(std::move(cid));
    }

    return cids;
  }
}  // namespace fc::codec::json

OUTCOME_CPP_DEFINE_CATEGORY(fc::codec::json, JsonCodecError, e) {
  using fc::codec::json::JsonCodecError;
  switch (e) {
    case JsonCodecError::BAD_JSON:
      return "Source data is not json document";
    case JsonCodecError::WRONG_CID_ARRAY_FORMAT:
      return "Failed to decode CID object";
  }
  return "Unknown error";
}
