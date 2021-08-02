/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/hamt/hamt.hpp"

namespace fc::adt {
  struct StringKeyer {
    using Key = BytesIn;

    inline static Bytes encode(const Key &key) {
      return copy(key);
    }

    inline static outcome::result<Key> decode(BytesIn key) {
      return key;
    }
  };

  /// Strongly typed hamt wrapper
  template <typename Value,
            typename Keyer = StringKeyer,
            size_t bit_width = storage::hamt::kDefaultBitWidth>
  struct Map {
    using Key = typename Keyer::Key;
    using Visitor =
        std::function<outcome::result<void>(const Key &, const Value &)>;

    Map(IpldPtr ipld = nullptr) : hamt{ipld, bit_width} {}

    Map(const CID &root, IpldPtr ipld = nullptr)
        : hamt{ipld, root, bit_width} {}

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
      return hamt.visit([&](auto key, auto value) -> outcome::result<void> {
        OUTCOME_TRY(key2, Keyer::decode(key));
        OUTCOME_TRY(value2, cbor_blake::cbDecodeT<Value>(hamt.ipld, value));
        return visitor(key2, value2);
      });
    }

    outcome::result<std::vector<Key>> keys() const {
      std::vector<Key> keys;
      OUTCOME_TRY(hamt.visit([&](auto key, auto) -> outcome::result<void> {
        OUTCOME_TRY(key2, Keyer::decode(key));
        keys.push_back(std::move(key2));
        return outcome::success();
      }));
      return keys;
    }

    outcome::result<size_t> size() const {
      size_t size{};
      OUTCOME_TRY(hamt.visit([&](auto key, auto) -> outcome::result<void> {
        ++size;
        return outcome::success();
      }));
      return size;
    }

    mutable storage::hamt::Hamt hamt;
  };

  /// Cbor encode map
  template <class Stream,
            typename Value,
            typename Keyer,
            size_t bit_width,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Map<Value, Keyer, bit_width> &map) {
    return s << map.hamt.cid();
  }

  /// Cbor decode map
  template <class Stream,
            typename Value,
            typename Keyer,
            size_t bit_width,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Map<Value, Keyer, bit_width> &map) {
    CID root;
    s >> root;
    map.hamt = {nullptr, root, bit_width};
    return s;
  }
}  // namespace fc::adt

namespace fc::cbor_blake {
  template <typename V, typename Keyer, size_t bit_width>
  struct CbLoadT<adt::Map<V, Keyer, bit_width>> {
    static void call(CbIpldPtrIn ipld, adt::Map<V, Keyer, bit_width> &map) {
      map.hamt.ipld = ipld;
    }
  };

  template <typename V, typename Keyer, size_t bit_width>
  struct CbFlushT<adt::Map<V, Keyer, bit_width>> {
    static auto call(adt::Map<V, Keyer, bit_width> &map) {
      return map.hamt.flush();
    }
  };
}  // namespace fc::cbor_blake
