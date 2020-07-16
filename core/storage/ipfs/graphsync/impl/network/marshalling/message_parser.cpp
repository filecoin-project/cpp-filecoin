/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_parser.hpp"

#include "codec/cbor/cbor_decode_stream.hpp"
#include "common/span.hpp"
#include "crypto/hasher/hasher.hpp"

#include "protobuf/message.pb.h"

namespace fc::storage::ipfs::graphsync {
  namespace {
    using codec::cbor::CborDecodeStream;

    // Dummy protobuf helper, string->bytes
    inline auto fromString(const std::string &src) {
      return common::Buffer{common::span::cbytes(src)};
    }

    // Checks status code received from wire
    outcome::result<ResponseStatusCode> extractStatusCode(int code) {
      switch (code) {
// clang-format off
#define CHECK_CASE(X) case RS_##X: return RS_##X;
        CHECK_CASE(FULL_CONTENT)
        CHECK_CASE(PARTIAL_CONTENT)
        CHECK_CASE(NOT_FOUND)
        CHECK_CASE(REQUEST_ACKNOWLEDGED)
        CHECK_CASE(ADDITIONAL_PEERS)
        CHECK_CASE(NOT_ENOUGH_GAS)
        CHECK_CASE(OTHER_PROTOCOL)
        CHECK_CASE(PARTIAL_RESPONSE)
        CHECK_CASE(REJECTED)
        CHECK_CASE(TRY_AGAIN)
        CHECK_CASE(REQUEST_FAILED)
        CHECK_CASE(LEGAL_ISSUES)
#undef CHECK_CASE
          // clang-format on

        default:
          break;
      }
      logger()->warn("{}: unknonwn status code {}", __FUNCTION__, code);
      return Error::kMessageParseError;
    }

    // Extracts requests from protobuf message
    outcome::result<void> parseRequests(pb::Message &pb_msg, Message &msg) {
      auto sz = pb_msg.requests_size();
      if (sz > 0) {
        msg.requests.reserve(sz);
        for (auto &src : pb_msg.requests()) {
          auto &dst = msg.requests.emplace_back(Message::Request());
          dst.id = src.id();
          if (src.cancel()) {
            dst.cancel = true;
          } else {
            OUTCOME_TRY(cid, CID::fromBytes(common::span::cbytes(src.root())));
            dst.root_cid = std::move(cid);
            dst.selector = fromString(src.selector());
            dst.priority = src.priority();

            for (const auto &[k, v] : src.extensions()) {
              std::vector<uint8_t> data(v.begin(), v.end());
              dst.extensions.emplace_back(Extension{k, data});
            }
          }
        }
      }
      return outcome::success();
    }

    // Extracts responses from protobuf message
    outcome::result<void> parseResponses(pb::Message &pb_msg, Message &msg) {
      size_t sz = pb_msg.responses_size();
      if (sz > 0) {
        msg.responses.reserve(sz);

        for (auto &src : pb_msg.responses()) {
          auto &dst = msg.responses.emplace_back(Message::Response());
          dst.id = src.id();
          OUTCOME_TRY(status, extractStatusCode(src.status()));
          dst.status = status;

          for (const auto &[k, v] : src.extensions()) {
            std::vector<uint8_t> data(v.begin(), v.end());
            dst.extensions.emplace_back(Extension{k, data});
          }
        }
      }
      return outcome::success();
    }

    // Extracts data blocks from protobuf message
    outcome::result<void> parseBlocks(pb::Message &pb_msg, Message &msg) {
      size_t sz = pb_msg.data_size();
      if (sz > 0) {
        msg.data.reserve(sz);

        for (auto &src : pb_msg.data()) {
          auto data = fromString(src.data());
          auto prefix_reader = common::span::cbytes(src.prefix());
          OUTCOME_TRY(cid, CID::read(prefix_reader, true));
          if (!prefix_reader.empty()) {
            return Error::kMessageParseError;
          }
          cid.content_address =
              crypto::Hasher::calculate(cid.content_address.getType(), data);
          msg.data.emplace_back(std::move(cid), data);
        }
      }
      return outcome::success();
    }

  }  // namespace

  outcome::result<Message> parseMessage(gsl::span<const uint8_t> bytes) {
    pb::Message pb_msg;

    if (!pb_msg.ParseFromArray(bytes.data(), bytes.size())) {
      logger()->warn("{}: cannot parse protobuf message, size={}",
                     __FUNCTION__,
                     bytes.size());
      return Error::kMessageParseError;
    }

    Message msg;

    msg.complete_request_list = pb_msg.completerequestlist();

    auto res = parseRequests(pb_msg, msg);
    if (!res) {
      return res.error();
    }

    res = parseResponses(pb_msg, msg);
    if (!res) {
      return res.error();
    }

    res = parseBlocks(pb_msg, msg);
    if (!res) {
      return res.error();
    }

    return msg;
  }

}  // namespace fc::storage::ipfs::graphsync
