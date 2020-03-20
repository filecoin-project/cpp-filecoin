/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/graphsync/extension.hpp"

#include "codec/cbor/cbor_encode_stream.hpp"
#include "codec/cbor/cbor_decode_stream.hpp"
#include "common.hpp"

namespace fc::storage::ipfs::graphsync {

  using codec::cbor::CborEncodeStream;
  using codec::cbor::CborDecodeStream;

  namespace {

    const std::string link("link");
    const std::string blockPresent("blockPresent");

    // encodes metadata item, {"link":CID, "blockPresent":bool}
    std::map<std::string, CborEncodeStream> encodeMetadataItem(
        const std::pair<CID, bool> &item) {
      std::map<std::string, CborEncodeStream> m;
      m[link] << item.first;
      m[blockPresent] << item.second;
      return m;
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

  }  // namespace

  Extension encodeMetadataRequest() {
    static const Extension e = {
      std::string(kResponseMetadataProtocol),
      std::vector<uint8_t>{0xF5}
    };
    return e;
  }

  Extension encodeResponseMetadata(const ResponseMetadata& metadata) {
    Extension e;
    e.name = kResponseMetadataProtocol;
    auto l = CborEncodeStream::list();
    for (const auto &item : metadata) {
      l << encodeMetadataItem(item);
    }
    CborEncodeStream encoder;
    encoder << l;
    e.data = encoder.data();
    return e;
  }

  outcome::result<ResponseMetadata> decodeResponseMetadata(
      const Extension &extension) {
    ResponseMetadata pairs;

    if (extension.name != kResponseMetadataProtocol) {
      return Error::MESSAGE_PARSE_ERROR;
    }

    if (extension.data.empty()) {
      return pairs;
    }

    try {
      CborDecodeStream decoder(extension.data);

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

  Extension encodeDontSendCids(const std::vector<CID> &dont_send_cids) {
    Extension e;
    e.name = kDontSendCidsProtocol;
    CborEncodeStream encoder;
    encoder << dont_send_cids;
    e.data = encoder.data();
    return e;
  }

  outcome::result<std::set<CID>> decodeDontSendCids(
      const Extension &extension) {
    if (extension.name != kDontSendCidsProtocol) {
      return Error::MESSAGE_PARSE_ERROR;
    }

    std::vector<CID> cids;
    try {
      CborDecodeStream decoder(extension.data);
      decoder >> cids;
    } catch (const std::exception &e) {
      logger()->warn("{}: {}", __FUNCTION__, e.what());
      return Error::MESSAGE_PARSE_ERROR;
    }
    return std::set<CID>(cids.begin(), cids.end());
  }

}  // namespace fc::storage::ipfs::graphsync
