/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/ipld.hpp"
#include "codec/cbor/light_reader/cid.hpp"

namespace fc::hamt_diff {
  using codec::cbor::CborToken;

  using Cb2 = std::function<bool(BytesIn key, BytesIn value1, BytesIn value2)>;
  using Cb1 = std::function<bool(BytesIn key, BytesIn value)>;

  template <bool kAddress, typename Cb>
  auto cbVarint(Cb &&cb) {
    return [cb{std::forward<Cb>(cb)}](
               BytesIn _key, BytesIn value1, BytesIn value2) mutable {
      if constexpr (kAddress) {
        if (_key.empty() || _key[0] != 0) {
          return false;
        }
        _key = _key.subspan(1);
      }
      uint64_t key{};
      if (!codec::uvarint::read(key, _key) || !_key.empty()) {
        return false;
      }
      return cb(key, value1, value2);
    };
  }

  using Bits64 = uint64_t;
  using Leaf = std::vector<std::pair<BytesIn, BytesIn>>;

  struct Bucket {
    const CbCid *shard{};
    Leaf leaf{};

    void reset() {
      shard = nullptr;
      leaf.resize(0);
    }
  };
  inline bool operator==(const Bucket &bucket1, const Bucket &bucket2) {
    if (bucket1.shard) {
      return bucket2.shard && *bucket1.shard == *bucket2.shard;
    }
    return !bucket2.shard && bucket1.leaf == bucket2.leaf;
  }

  struct Node {
    Bytes buf;
    BytesIn input;
    Bits64 bits{};
    size_t buckets{};

    bool init(const CbIpldPtr &ipld, const CbCid &cid) {
      if (!ipld->get(cid, buf)) {
        return false;
      }
      input = buf;
      CborToken token;
      if (read(token, input).listCount() != 2) {
        return false;
      }
      if (!read(token, input).bytesSize()) {
        return false;
      }
      BytesIn _bits;
      if (!codec::read(_bits, input, *token.bytesSize())) {
        return false;
      }
      if (static_cast<size_t>(_bits.size()) > sizeof(Bits64)) {
        return false;
      }
      for (const auto &byte : _bits) {
        bits = (bits << 8) | byte;
      }
      if (!read(token, input).listCount()) {
        return false;
      }
      buckets = *token.listCount();
      return true;
    }
    bool bucket(Bucket &bucket) {
      bucket.reset();
      if (buckets == 0) {
        return false;
      }
      --buckets;
      CborToken token;
      if (!read(token, input)) {
        return false;
      }
      if (token.mapCount()) {
        if (token.mapCount() != 1) {
          return false;
        }
        if (read(token, input).strSize() != 1) {
          return false;
        }
        BytesIn str;
        if (!codec::read(str, input, 1) || (str[0] != '0' && str[0] != '1')) {
          return false;
        }
        if (!read(token, input)) {
          return false;
        }
      }
      if (token.cidSize()) {
        if (!readCborBlake(bucket.shard, token, input)) {
          return false;
        }
      } else {
        if (!token.listCount()) {
          return false;
        }
        auto n{*token.listCount()};
        bucket.leaf.reserve(n);
        while (n) {
          --n;
          BytesIn key, value;
          if (read(token, input).listCount() != 2) {
            return false;
          }
          if (!read(token, input).bytesSize()
              || !codec::read(key, input, *token.bytesSize())) {
            return false;
          }
          if (!codec::cbor::readNested(value, input)) {
            return false;
          }
          bucket.leaf.emplace_back(key, value);
        }
      }
      return true;
    }
  };

  inline bool hamtVisit(const CbIpldPtr &ipld, Bucket &bucket, const Cb1 &cb) {
    if (!bucket.shard) {
      for (const auto &[key, value] : bucket.leaf) {
        if (!cb(key, value)) {
          return false;
        }
      }
      return true;
    }
    Node node;
    if (!node.init(ipld, *bucket.shard)) {
      return false;
    }
    while (node.buckets != 0) {
      if (!node.bucket(bucket)) {
        return false;
      }
      if (!hamtVisit(ipld, bucket, cb)) {
        return false;
      }
    }
    return true;
  }

  inline bool hamtDiff(const CbIpldPtr &ipld,
                       Bucket &bucket1,
                       Bucket &bucket2,
                       const Cb2 &cb);

  inline bool hamtDiffShard(const CbIpldPtr &ipld,
                            Bucket &bucket1,
                            Bucket &bucket2,
                            const Cb2 &cb) {
    Node node1, node2;
    if (!node1.init(ipld, *bucket1.shard)) {
      return false;
    }
    if (!node2.init(ipld, *bucket2.shard)) {
      return false;
    }
    Bits64 mask{node1.bits | node2.bits};
    for (Bits64 bit{1}; mask != 0; bit <<= 1, mask >>= 1) {
      if ((mask & 1) == 0) {
        continue;
      }
      bucket1.reset();
      bucket2.reset();
      if ((node1.bits & bit) != 0) {
        if (!node1.bucket(bucket1)) {
          return false;
        }
      }
      if ((node2.bits & bit) != 0) {
        if (!node2.bucket(bucket2)) {
          return false;
        }
      }
      if (bucket1 == bucket2) {
        continue;
      }
      if (!hamtDiff(ipld, bucket1, bucket2, cb)) {
        return false;
      }
    }
    return true;
  }

  bool hamtDiff(const CbIpldPtr &ipld,
                Bucket &bucket1,
                Bucket &bucket2,
                const Cb2 &cb) {
    if (bucket1.shard && bucket2.shard) {
      return hamtDiffShard(ipld, bucket1, bucket2, cb);
    }
    if (bucket2.shard) {
      return hamtDiff(ipld,
                      bucket2,
                      bucket1,
                      [&](BytesIn key, BytesIn value1, BytesIn value2) {
                        return cb(key, value2, value1);
                      });
    }
    const auto visit{[&](BytesIn key, BytesIn value1) {
      BytesIn value2;
      for (auto &p : bucket2.leaf) {
        if (p.first == key) {
          std::swap(p.second, value2);
          break;
        }
      }
      if (value1 == value2) {
        return true;
      }
      return cb(key, value1, value2);
    }};
    if (!hamtVisit(ipld, bucket1, visit)) {
      return false;
    }
    for (const auto &[key, value2] : bucket2.leaf) {
      if (!value2.empty()) {
        if (!cb(key, {}, value2)) {
          return false;
        }
      }
    }
    return true;
  }

  inline bool hamtDiff(const CbIpldPtr &ipld,
                       const CbCid &cid1,
                       const CbCid &cid2,
                       const Cb2 &cb) {
    if (cid1 == cid2) {
      return true;
    }
    Bucket bucket1{&cid1}, bucket2{&cid2};
    return hamtDiffShard(ipld, bucket1, bucket2, cb);
  }
}  // namespace fc::hamt_diff
