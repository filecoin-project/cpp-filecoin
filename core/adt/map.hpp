/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/hamt/hamt.hpp"

namespace fc::adt {
  using storage::hamt::Hamt;

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
            size_t bit_width = storage::hamt::kDefaultBitWidth,
            bool v3 = false>
  struct Map {
    using Key = typename Keyer::Key;
    using Visitor =
        std::function<outcome::result<void>(const Key &, const Value &)>;

    Map(IpldPtr ipld = nullptr) : hamt{ipld, bit_width, v3} {}

    Map(const CID &root, IpldPtr ipld = nullptr)
        : hamt{ipld, root, bit_width, v3} {}

    outcome::result<boost::optional<Value>> tryGet(const Key &key) const {
      return hamt.tryGetCbor<Value>(Keyer::encode(key));
    }

    outcome::result<bool> has(const Key &key) const {
      return hamt.contains(Keyer::encode(key));
    }

    outcome::result<Value> get(const Key &key) const {
      return hamt.getCbor<Value>(Keyer::encode(key));
    }

    outcome::result<void> set(const Key &key, const Value &value) {
      return hamt.setCbor(Keyer::encode(key), value);
    }

    outcome::result<void> remove(const Key &key) {
      return hamt.remove(Keyer::encode(key));
    }

    outcome::result<void> visit(const Visitor &visitor) const {
      return hamt.visit([&](auto &key, auto &value) -> outcome::result<void> {
        OUTCOME_TRY(key2, Keyer::decode(key));
        OUTCOME_TRY(value2, hamt.ipld->decode<Value>(value));
        return visitor(key2, value2);
      });
    }

    outcome::result<std::vector<Key>> keys() const {
      std::vector<Key> keys;
      OUTCOME_TRY(hamt.visit([&](auto &key, auto &) -> outcome::result<void> {
        OUTCOME_TRY(key2, Keyer::decode(key));
        keys.push_back(std::move(key2));
        return outcome::success();
      }));
      return keys;
    }

    outcome::result<size_t> size() const {
      size_t size{};
      OUTCOME_TRY(hamt.visit([&](auto &key, auto &) -> outcome::result<void> {
        ++size;
        return outcome::success();
      }));
      return size;
    }

    mutable Hamt hamt;
  };

  template <typename Value,
            typename Keyer = StringKeyer,
            size_t bit_width = storage::hamt::kDefaultBitWidth>
  using MapV3 = Map<Value, Keyer, bit_width, true>;

  /// Cbor encode map
  template <class Stream,
            typename Value,
            typename Keyer,
            size_t bit_width,
            bool v3,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Map<Value, Keyer, bit_width, v3> &map) {
    return s << map.hamt.cid();
  }

  /// Cbor decode map
  template <class Stream,
            typename Value,
            typename Keyer,
            size_t bit_width,
            bool v3,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Map<Value, Keyer, bit_width, v3> &map) {
    CID root;
    s >> root;
    map.hamt = {nullptr, root, bit_width, v3};
    return s;
  }
}  // namespace fc::adt

namespace fc {
  template <typename V, typename Keyer, size_t bit_width, bool v3>
  struct Ipld::Load<adt::Map<V, Keyer, bit_width, v3>> {
    static void call(Ipld &ipld, adt::Map<V, Keyer, bit_width, v3> &map) {
      map.hamt.ipld = ipld.shared();
    }
  };

  template <typename V, typename Keyer, size_t bit_width, bool v3>
  struct Ipld::Flush<adt::Map<V, Keyer, bit_width, v3>> {
    static auto call(adt::Map<V, Keyer, bit_width, v3> &map) {
      return map.hamt.flush();
    }
  };
}  // namespace fc
