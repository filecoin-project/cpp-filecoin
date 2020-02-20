/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_parser.hpp"

#include "codec/cbor/cbor_decode_stream.hpp"
#include "storage/ipfs/graphsync/impl/errors.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"

#include <protobuf/message.pb.h>

namespace fc::storage::ipfs::graphsync {

  using codec::cbor::CborDecodeStream;

  namespace {

    common::Buffer fromString(const std::string &src) {
      common::Buffer dst;
      if (!src.empty()) {
        auto b = (const uint8_t*)src.data();
        auto e = b + src.size();
        dst.putBytes(b, e);
      }
      return dst;
    }

    outcome::result<ResponseStatusCode> extractStatusCode(
        int src) {
      switch (src) {
// clang-format off
#define CHECK_CASE(X) case RS_##X: return RS_##X;
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

    bool decodeBool(const std::string& s) {
      bool b = false;
      try {
        auto data = (const uint8_t*)s.data(); //NOLINT
        CborDecodeStream decoder(gsl::span<const uint8_t>(data, s.size()));
        decoder >> b;
      } catch (const std::exception& e) {
        // XXX log
      }
      return b;
    }

    std::vector<CID> decodeCids(const std::string& s) {
      std::vector<CID> cids;
      try {
        auto data = (const uint8_t*)s.data(); //NOLINT
        CborDecodeStream decoder(gsl::span<const uint8_t>(data, s.size()));
        decoder >> cids;
      } catch (const std::exception& e) {
        // XXX log
      }
      return cids;
    }

    outcome::result<CID> decodeCid(const std::string& s) {
      CID cid;
      try {
        auto data = (const uint8_t*)s.data(); //NOLINT
        CborDecodeStream decoder(gsl::span<const uint8_t>(data, s.size()));
        decoder >> cid;
      } catch (const std::exception& e) {
        // XXX log
        return Error::MESSAGE_PARSE_ERROR;
      }
      return cid;
    }

    outcome::result<ResponseMetadata> decodeMetadata(const std::string& s) {
      static const std::string link(kLink);
      static const std::string blockPresent(kBlockPresent);

      ResponseMetadata pairs;

      if (s.empty()) {
        return pairs;
      }

      try {
        auto data = (const uint8_t*)s.data(); //NOLINT
        CborDecodeStream decoder(gsl::span<const uint8_t>(data, s.size()));
        decoder = decoder.list();
        size_t n = decoder.listLength();
        pairs.reserve(n);

        for (size_t i=0; i<n; ++i) {
          auto m = decoder.map();
          auto link_p = m.find(link);
          auto present_p = m.find(blockPresent);
          if (link_p == m.end() || present_p == m.end()) {
            // XXX log
            return Error::MESSAGE_PARSE_ERROR;
          }
          CID cid;
          link_p->second >> cid;
          bool present = false;
          present_p->second >> present;

          pairs.push_back( { std::move(cid), present } );
        }
      } catch (const std::exception& e) {
        // XXX log
        return Error::MESSAGE_PARSE_ERROR;
      }

      return pairs;
    }

  }  // namespace

  outcome::result<Message> parseMessage(
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
            msg.requests.emplace_back(Message::Request());
        dst.id = src.id();
        if (src.cancel()) {
          dst.cancel = true;
        } else {
          dst.root_cid = fromString(src.root());
          dst.selector = fromString(src.selector());
          dst.priority = src.priority();

          for (const auto& [k, v] : src.extensions()) {
            if (k == kResponseMetadata) {
              dst.send_metadata = decodeBool(v);
              continue;
            }

            if (k == kDontSendCids) {
              dst.do_not_send = decodeCids(v);
              continue;
            }
            // XXX log unknown extension

          }
        }
      }
    }

    sz = pb_msg.responses_size();
    if (sz > 0) {
      msg.responses.reserve(sz);

      for (auto &src : pb_msg.responses()) {
        auto &dst =
            msg.responses.emplace_back(Message::Response());
        dst.id = src.id();
        auto res = extractStatusCode(src.status());
        if (!res) {
          return libp2p::outcome::failure(res.error());
        }
        dst.status = res.value();

        for (const auto& [k,v] : src.extensions()) {
          if (k == kResponseMetadata) {
            auto meta_res = decodeMetadata(v);

            if (meta_res) {
              dst.metadata = std::move(meta_res.value());
            } else {
              // XXX log
            }
            continue;
          }
          // XXX log unknown extension

        }
      }
    }

    sz = pb_msg.data_size();
    if (sz > 0) {
      msg.data.reserve(sz);

      for (auto &src : pb_msg.data()) {
        auto res = decodeCid(src.prefix());
        if (!res) {
          return outcome::failure(res.error());
        }
        msg.data.push_back( {std::move(res.value()), fromString(src.data()) } );
      }
    }

    return msg;
  }

}  // namespace fc::storage::ipfs::graphsync
