/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/make.hpp"

#include <boost/algorithm/string.hpp>

#include "codec/cbor/cbor_resolve.hpp"

namespace fc::api {
  using boost::starts_with;
  using codec::cbor::CborDecodeStream;
  using codec::cbor::parseIndex;
  using storage::amt::Amt;
  using storage::hamt::Hamt;

  outcome::result<IpldObject> getNode(std::shared_ptr<Ipld> ipld,
                                      const CID &root,
                                      gsl::span<const std::string> parts) {
    if (root.content_type != libp2p::multi::MulticodecType::DAG_CBOR) {
      return TodoError::kError;
    }
    OUTCOME_TRY(raw, ipld->get(root));
    try {
      CborDecodeStream s{raw};
      for (auto i = 0u; i < parts.size(); ++i) {
        auto &part = parts[i];
        auto a = starts_with(part, "@A:");
        auto hi = starts_with(part, "@Hi:");
        auto hu = starts_with(part, "@Hu:");
        auto ha = starts_with(part, "@Ha:");
        auto h = starts_with(part, "@H:");
        if (a || hi || hu || ha || h) {
          raw = Buffer{s.raw()};
          OUTCOME_TRY(cid, common::getCidOf(raw));
          if (a) {
            OUTCOME_TRY(index, parseIndex(part.substr(3)));
            OUTCOME_TRY(raw2, Amt{ipld, cid}.get(index));
            s = CborDecodeStream{raw2};
          } else {
            std::string key;
            if (hi) {
              auto neg = part.size() >= 4 && part[4] == '-';
              OUTCOME_TRY(value, parseIndex(part.substr(neg ? 5 : 4)));
              key = adt::VarintKeyer::encode(neg ? -value : value);
            } else if (hu) {
              OUTCOME_TRY(value, parseIndex(part.substr(4)));
              key = adt::UvarintKeyer::encode(value);
            } else if (ha) {
              OUTCOME_TRY(
                  address,
                  primitives::address::decodeFromString(part.substr(4)));
              key = adt::AddressKeyer::encode(address);
            } else {
              key = part.substr(3);
            }
            OUTCOME_TRY(raw2, Hamt{ipld, cid}.get(key));
            s = CborDecodeStream{raw2};
          }
          parts = parts.subspan(1);
          break;
        } else {
          OUTCOME_TRY(codec::cbor::resolve(s, part));
        }
        if (s.isCid()) {
          auto s2 = s;
          CID cid;
          s2 >> cid;
          if (cid.content_type == libp2p::multi::MulticodecType::DAG_CBOR) {
            OUTCOME_TRY(raw2, ipld->get(cid));
            s = CborDecodeStream{raw2};
          } else {
            if (i != parts.size() - 1) {
              return TodoError::kError;
            }
          }
        }
      }
      raw = Buffer{s.raw()};
      OUTCOME_TRY(cid, common::getCidOf(raw));
      return IpldObject{std::move(cid), std::move(raw)};
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
  }
}  // namespace fc::api
