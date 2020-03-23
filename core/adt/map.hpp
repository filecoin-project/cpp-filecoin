/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_MAP_HPP
#define CPP_FILECOIN_ADT_MAP_HPP

#include "storage/hamt/hamt.hpp"

namespace fc::adt {
  using storage::hamt::Hamt;
  using storage::hamt::kDefaultBitWidth;
  using Ipld = storage::ipfs::IpfsDatastore;

  struct StringKeyer {
    using Key = std::string;

    inline static std::string encode(const Key &key) {
      return key;
    }

    inline static outcome::result<Key> decode(const std::string &key) {
      return key;
    }
  };

  /// Strongly typed hamt wrapper
  template <typename Value,
            typename Keyer = StringKeyer,
            size_t bit_width = kDefaultBitWidth>
  struct Map {
    using Key = typename Keyer::Key;
    using Visitor =
        std::function<outcome::result<void>(const Key &, const Value &)>;

    Map() : hamt{nullptr, bit_width} {}

    explicit Map(const CID &root) : hamt{nullptr, root, bit_width} {}

    void load(std::shared_ptr<Ipld> ipld) {
      hamt.setIpld(ipld);
    }

    outcome::result<boost::optional<Value>> tryGet(const Key &key) {
      return hamt.tryGetCbor<Value>(Keyer::encode(key));
    }

    outcome::result<bool> has(const Key &key) {
      return hamt.contains(Keyer::encode(key));
    }

    outcome::result<Value> get(const Key &key) {
      return hamt.getCbor<Value>(Keyer::encode(key));
    }

    outcome::result<void> set(const Key &key, const Value &value) {
      return hamt.setCbor(Keyer::encode(key), value);
    }

    outcome::result<void> remove(const Key &key) {
      return hamt.remove(Keyer::encode(key));
    }

    outcome::result<void> flush() {
      OUTCOME_TRY(hamt.flush());
      return outcome::success();
    }

    outcome::result<void> visit(const Visitor &visitor) {
      return hamt.visit([&](auto &key, auto &value) -> outcome::result<void> {
        OUTCOME_TRY(key2, Keyer::decode(key));
        OUTCOME_TRY(value2, codec::cbor::decode<Value>(value));
        return visitor(key2, value2);
      });
    }

    Hamt hamt;
  };

  /// Cbor encode map
  template <class Stream,
            typename V,
            typename K,
            size_t bit_width,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Map<V, K, bit_width> &map) {
    return s << map.hamt.cid();
  }

  /// Cbor decode map
  template <class Stream,
            typename V,
            typename K,
            size_t bit_width,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Map<V, K, bit_width> &map) {
    CID root;
    s >> root;
    map.hamt = {nullptr, root, bit_width};
    return s;
  }
}  // namespace fc::adt

#endif  // CPP_FILECOIN_ADT_MAP_HPP
