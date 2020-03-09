/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/cid/json_codec.hpp"

#include <strstream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "filecoin/codec/cbor/cbor.hpp"

namespace fc::codec::json {
  using boost::property_tree::ptree;

  namespace {
    using libp2p::multi::detail::decodeBase58;
    using libp2p::multi::detail::encodeBase58;

    // TODO(yuraz): FIL-156 implement correct encode and decode
    // this solution is temporary and not compatible with lotus implementation
    // need to decide whether we need implement it according to lotus
    outcome::result<std::string> encodeCid(const CID &cid) {
      std::string result;
      OUTCOME_TRY(cid_bytes, cbor::encode(cid));
      return encodeBase58(cid_bytes);
    }

    outcome::result<CID> decodeCid(std::string_view data) {
      OUTCOME_TRY(bytes, decodeBase58(data));
      return cbor::decode<CID>(bytes);
    }
  }  // namespace

  outcome::result<std::string> encodeCidVector(gsl::span<const CID> span) {
    ptree tree;
    ptree children;
    for (const auto &it : span) {
      OUTCOME_TRY(encoded, encodeCid(it));
      ptree child;
      child.put("", encoded);
      children.push_back(std::make_pair("", child));
    }

    tree.add_child("/", children);

    // get json from tree
    std::stringstream out;
    boost::property_tree::write_json(out, tree);

    return out.str();
  }

  outcome::result<std::vector<CID>> decodeCidVector(std::string_view data) {
    ptree tree;
    std::stringstream stream;
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
      OUTCOME_TRY(cid, decodeCid(val));
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
