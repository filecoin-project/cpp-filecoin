/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_parser.hpp"

#include "codec/cbor/cbor_decode_stream.hpp"

#include "protobuf/message.pb.h"

namespace fc::storage::ipfs::graphsync {
  namespace {
    using codec::cbor::CborDecodeStream;

    // Dummy protobuf helper, string->bytes
    common::Buffer fromString(const std::string &src) {
      common::Buffer dst;
      if (!src.empty()) {
        auto b = (const uint8_t *)src.data();
        auto e = b + src.size();
        dst.putBytes(b, e);
      }
      return dst;
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
      return Error::MESSAGE_PARSE_ERROR;
    }

    // Decodes cbor boolean from byte
    outcome::result<bool> decodeBool(uint8_t byte) {
      constexpr uint8_t kCborFalse = 0xF4;
      constexpr uint8_t kCborTrue = 0xF5;
      if (byte == kCborFalse) {
        return false;
      } else if (byte == kCborTrue) {
        return true;
      }
      logger()->warn("{}: wrong cbor encoding: bool expected", __FUNCTION__);
      return Error::MESSAGE_PARSE_ERROR;
    }

    // Decodes cbor boolean from string or byte array
    template <typename Container>
    outcome::result<bool> decodeBool(const Container &s) {
      if (s.size() == 1) {
        return decodeBool((uint8_t)s[0]);
      }
      logger()->warn("{}: wrong cbor encoding: single byte expected",
                     __FUNCTION__);
      return Error::MESSAGE_PARSE_ERROR;
    }

    // Decodes list of CIDs from CBOR source
    outcome::result<std::vector<CID>> decodeCids(const std::string &s) {
      std::vector<CID> cids;
      try {
        auto data = (const uint8_t *)s.data();  // NOLINT
        CborDecodeStream decoder(gsl::span<const uint8_t>(data, s.size()));
        decoder >> cids;
      } catch (const std::exception &e) {
        logger()->warn("{}: {}", __FUNCTION__, e.what());
        return Error::MESSAGE_PARSE_ERROR;
      }
      return cids;
    }

    // Decodes CID from CBOR source
    outcome::result<CID> decodeCid(const std::string &s) {
      CID cid;
      try {
        auto data = (const uint8_t *)s.data();  // NOLINT
        CborDecodeStream decoder(gsl::span<const uint8_t>(data, s.size()));
        decoder >> cid;
      } catch (const std::exception &e) {
        logger()->warn("{}: {}", __FUNCTION__, e.what());
        return Error::MESSAGE_PARSE_ERROR;
      }
      return cid;
    }

    // Decodes metadata from CBOR source with such a schema
    // [ {"link":CID, "blockPresent":bool}, ...]
    outcome::result<ResponseMetadata> decodeMetadata(const std::string &s) {
      static const std::string link(kLink);
      static const std::string blockPresent(kBlockPresent);

      ResponseMetadata pairs;

      if (s.empty()) {
        return pairs;
      }

      try {
        auto data = (const uint8_t *)s.data();  // NOLINT
        CborDecodeStream decoder(gsl::span<const uint8_t>(data, s.size()));

        if (!decoder.isList()) {
          logger()->warn("{}: wrong cbor encoding: not a list", __FUNCTION__);
          return Error::MESSAGE_PARSE_ERROR;
        }
        size_t n = decoder.listLength();
        pairs.reserve(n);

        decoder = decoder.list();

        std::map<std::string, CborDecodeStream> m;
        std::vector<uint8_t> raw;

        for (size_t i = 0; i < n; ++i) {
          raw = decoder.raw();
          CborDecodeStream x(raw);
          if (!x.isMap()) {
            logger()->warn("{}: wrong cbor encoding: entry is not a map",
                           __FUNCTION__);
            return Error::MESSAGE_PARSE_ERROR;
          }

          m = x.map();

          auto link_p = m.find(link);
          auto present_p = m.find(blockPresent);
          if (link_p == m.end() || present_p == m.end()) {
            logger()->warn("{}: wrong cbor encoding: required fields missing",
                           __FUNCTION__);
            return Error::MESSAGE_PARSE_ERROR;
          }

          CID cid;
          link_p->second >> cid;

          OUTCOME_TRY(present, decodeBool(present_p->second.raw()));

          pairs.push_back({std::move(cid), present});
        }
      } catch (const std::exception &e) {
        logger()->warn("{}: {}", __FUNCTION__, e.what());
        return Error::MESSAGE_PARSE_ERROR;
      }

      return pairs;
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
            OUTCOME_TRY(cid, decodeCid(src.root()));
            dst.root_cid = std::move(cid);
            dst.selector = fromString(src.selector());
            dst.priority = src.priority();

            for (const auto &[k, v] : src.extensions()) {
              if (k == kResponseMetadata) {
                OUTCOME_TRY(send_metadata, decodeBool(v));
                dst.send_metadata = send_metadata;
                continue;
              }

              if (k == kDontSendCids) {
                OUTCOME_TRY(cids, decodeCids(v));
                dst.do_not_send = std::move(cids);
                continue;
              }

              logger()->debug(
                  "{}: unknown extension name: {}", __FUNCTION__, k);
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
            if (k == kResponseMetadata) {
              OUTCOME_TRY(metadata, decodeMetadata(v));
              dst.metadata = std::move(metadata);
              continue;
            }
            logger()->debug("{}: unknown extension name: {}", __FUNCTION__, k);
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
          OUTCOME_TRY(cid, decodeCid(src.prefix()));
          msg.data.emplace_back(std::move(cid), fromString(src.data()));
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
      return Error::MESSAGE_PARSE_ERROR;
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
