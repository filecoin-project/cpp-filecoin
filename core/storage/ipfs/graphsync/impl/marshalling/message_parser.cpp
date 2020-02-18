/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_parser.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/uvarint.hpp>

#include "../errors.hpp"

#include <protobuf/message.pb.h>

namespace fc::storage::ipfs::graphsync {

  namespace {

    ByteArray fromString(const std::string &src) {
      ByteArray dst;
      auto sz = src.size();
      dst.resize(sz);
      if (sz > 0) {
        memcpy(dst.data(), src.data(), sz);
      }
      return dst;
    }

    libp2p::outcome::result<Message::ResponseStatusCode> extractStatusCode(
        int src) {
      switch (src) {
// clang-format off
#define CHECK_CASE(X) case Message::RS_##X: return Message::RS_##X;
        CHECK_CASE(REQUEST_ACKNOWLEDGED)
        CHECK_CASE(ADDITIONAL_PEERS)
        CHECK_CASE(NOT_ENOUGH_GAS)
        CHECK_CASE(OTHER_PROTOCOL)
        CHECK_CASE(PARTIAL_RESPONSE)
        CHECK_CASE(FULL_CONTENT)
        CHECK_CASE(PARTIAL_CONTENT)
        CHECK_CASE(REJECTED)
        CHECK_CASE(TRY_AGAIN)
        CHECK_CASE(REQUEST_FAILED)
        CHECK_CASE(LEGAL_ISSUES)
        CHECK_CASE(NOT_FOUND)
#undef CHECK_CASE
          // clang-format on

        default:
          break;
      }
      return Error::MESSAGE_VALIDATION_FAILED;
    }

    libp2p::outcome::result<ByteArray> extractCID(const std::string &prefix,
                                                  const std::string &data) {
      if (prefix.empty()) {
        return Error::MESSAGE_PARSE_ERROR;
      }
      if (prefix.size() >= 2 && prefix[0] == 0x12 && prefix[1] == 0x20) {
        // CIDv0
        return libp2p::multi::ContentIdentifierCodec::encodeCIDV0(data.data(),
                                                                  data.size());
      }
      if (prefix[0] == 1) {
        // CIDv1

        // TODO(FIL-96): hash types other than sha256

        // XXX

      }
      return Error::MESSAGE_PARSE_ERROR;

    }

  }  // namespace

  libp2p::outcome::result<Message> parseMessage(
      gsl::span<const uint8_t> bytes) {
    pb::Message pb_msg;
    if (!pb_msg.ParseFromArray(bytes.data(), bytes.size())) {
      return Error::MESSAGE_PARSE_ERROR;
    }

    Message msg;

    msg.complete_request_list = pb_msg.completerequestlist();

    auto sz = pb_msg.requests_size();
    if (sz > 0) {
      msg.requests.reserve(sz);
      for (auto &src : pb_msg.requests()) {
        auto &dst =
            msg.requests.emplace_back(std::make_shared<Message::Request>());
        dst->id = src.id();
        dst->root_cid = fromString(src.root());
        dst->selector = src.selector();
        // TODO(FIL-96): src.extensions; src.priority;
        dst->cancel = src.cancel();
      }
    }

    sz = pb_msg.responses_size();
    if (sz > 0) {
      msg.responses.reserve(sz);
      for (auto &src : pb_msg.responses()) {
        auto &dst =
            msg.responses.emplace_back(std::make_shared<Message::Response>());
        dst->id = src.id();
        auto res = extractStatusCode(src.status());
        if (!res) {
          return libp2p::outcome::failure(res.error());
        }
        dst->status = res.value();

        // TODO(FIL-96): src.metadata;
      }
    }

    sz = pb_msg.data_size();
    if (sz > 0) {
      for (auto &src : pb_msg.data()) {
        auto res = extractCID(src.prefix(), src.data());
        if (!res) {
          return libp2p::outcome::failure(res.error());
        }
        msg.data[std::move(res.value())] = fromString(src.data());
      }
    }

    return msg;
  }

}  // namespace fc::storage::ipfs::graphsync
