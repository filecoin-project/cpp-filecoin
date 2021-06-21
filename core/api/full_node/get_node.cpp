/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/full_node/make.hpp"

#include <boost/algorithm/string.hpp>

#include "adt/address_key.hpp"
#include "adt/uvarint_key.hpp"
#include "codec/cbor/cbor_resolve.hpp"
#include "storage/amt/amt.hpp"
#include "storage/hamt/hamt.hpp"

namespace fc::api {
  using boost::starts_with;
  using codec::cbor::CborDecodeStream;
  using codec::cbor::parseIndex;
  using storage::amt::Amt;
  using storage::hamt::Hamt;

  outcome::result<IpldObject> getNode(std::shared_ptr<Ipld> ipld,
                                      const CID &root,
                                      gsl::span<const std::string> parts) {
    static const auto error_dag_cbor{
        ERROR_TEXT("getNode: cid is not dag-cbor")};
    if (root.content_type != CID::Multicodec::DAG_CBOR) {
      return error_dag_cbor;
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
            OUTCOME_TRYA(raw, Amt{ipld, cid}.get(index));
            s = CborDecodeStream{raw};
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
            OUTCOME_TRYA(
                raw,
                Hamt{ipld, cid, storage::hamt::kDefaultBitWidth, false}.get(
                    key));
            s = CborDecodeStream{raw};
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
          if (cid.content_type == CID::Multicodec::DAG_CBOR) {
            OUTCOME_TRYA(raw, ipld->get(cid));
            s = CborDecodeStream{raw};
          } else {
            if (i != parts.size() - 1) {
              return error_dag_cbor;
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
