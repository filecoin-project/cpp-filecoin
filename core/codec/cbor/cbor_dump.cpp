/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_dump.hpp"

#include "codec/cbor/cbor_decode_stream.hpp"
#include "common/hexutil.hpp"

namespace fc {
  std::string dumpBytes(BytesIn bytes) {
    return common::hex_lower(bytes);
  }

  std::string dumpCid(const CID &cid) {
    const auto &mh{cid.content_address};
    auto hash{mh.getHash()};
    if (cid.version == CID::Version::V1
        && cid.content_type == CID::Multicodec::DAG_CBOR
        && mh.getType() == libp2p::multi::HashType::blake2b_256) {
      return dumpBytes(hash);
    }
    OUTCOME_EXCEPT(bytes, cid.toBytes());
    return dumpBytes(gsl::make_span(bytes).first(
               static_cast<ptrdiff_t>(bytes.size() - hash.size() - 1)))
           + "_" + dumpBytes(hash);
  }

  struct LessCborKey {
    bool operator()(const std::string &l, const std::string &r) const {
      return less(
          l.size(), r.size(), common::span::cbytes(l), common::span::cbytes(r));
    }
  };

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void dumpCbor(std::string &o, codec::cbor::CborDecodeStream &s) {
    auto dumpString{[&](auto &s) { o += "^" + s; }};
    if (s.isCid()) {
      CID cid;
      s >> cid;
      o += "@" + dumpCid(cid);
    } else if (s.isList()) {
      o += "[";
      auto n{s.listLength()};
      auto list{s.list()};
      for (auto i{0u}; i < n; ++i) {
        if (i != 0) {
          o += ",";
        }
        dumpCbor(o, list);
      }
      o += "]";
    } else if (s.isMap()) {
      o += "{";
      auto comma{false};
      auto _map{s.map()};
      std::map<std::string, codec::cbor::CborDecodeStream, LessCborKey> map{
          _map.begin(), _map.end()};
      for (auto &p : map) {
        if (comma) {
          o += ",";
        } else {
          comma = true;
        }
        dumpString(p.first);
        o += ":";
        dumpCbor(o, p.second);
      }
      o += "}";
    } else if (s.isBytes()) {
      Bytes bytes;
      s >> bytes;
      o += bytes.empty() ? "~" : dumpBytes(bytes);
    } else if (s.isStr()) {
      std::string str;
      s >> str;
      dumpString(str);
    } else if (s.isInt()) {
      int64_t i{};
      s >> i;
      if (i >= 0) {
        o += "+";
      }
      o += std::to_string(i);
    } else if (s.isBool()) {
      bool b{};
      s >> b;
      o += b ? "T" : "F";
    } else if (s.isNull()) {
      o += "N";
      s.next();
    } else {
      assert(false);
    }
  }

  std::string dumpCbor(BytesIn bytes) {
    if (bytes.empty()) {
      return "(empty)";
    }
    try {
      std::string o;
      codec::cbor::CborDecodeStream s{bytes};
      dumpCbor(o, s);
      return o;
    } catch (std::system_error &) {
      return "(error:" + dumpBytes(bytes) + ")";
    }
  }
}  // namespace fc
