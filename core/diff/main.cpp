/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <fmt/ostream.h>
#include <climits>
#include <filesystem>

#include "cbor_blake/ipld_any.hpp"
#include "cbor_blake/memory.hpp"
#include "codec/cbor/cbor_dump.hpp"
#include "codec/cbor/light_reader/address.hpp"
#include "codec/cbor/light_reader/cid.hpp"
#include "codec/cbor/light_reader/hamt_walk.hpp"
#include "common/file.hpp"
#include "common/prometheus/since.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/tipset/load.hpp"
#include "storage/car/car.hpp"
#include "storage/car/cids_index/util.hpp"
#include "storage/hamt/diff_rle.hpp"
#include "vm/actor/codes.hpp"

namespace fc::hamt_diff {
  using Set = std::set<CbCid>;

  inline bool hamtVisit(const CbIpldPtr &ipld, Bucket &bucket, Set &set) {
    if (!bucket.shard) {
      return true;
    }
    Node node;
    if (!node.init(ipld, *bucket.shard)) {
      return false;
    }
    set.insert(*bucket.shard);
    while (node.buckets != 0) {
      if (!node.bucket(bucket)) {
        return false;
      }
      if (!hamtVisit(ipld, bucket, set)) {
        return false;
      }
    }
    return true;
  }

  inline bool hamtDiff(const CbIpldPtr &ipld,
                       Bucket &bucket1,
                       Bucket &bucket2,
                       Set &set1,
                       Set &set2);

  inline bool hamtDiffShard(const CbIpldPtr &ipld,
                            Bucket &bucket1,
                            Bucket &bucket2,
                            Set &set1,
                            Set &set2) {
    Node node1, node2;
    if (!node1.init(ipld, *bucket1.shard)) {
      return false;
    }
    set1.insert(*bucket1.shard);
    if (!node2.init(ipld, *bucket2.shard)) {
      return false;
    }
    set2.insert(*bucket2.shard);
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
      if (!hamtDiff(ipld, bucket1, bucket2, set1, set2)) {
        return false;
      }
    }
    return true;
  }

  bool hamtDiff(const CbIpldPtr &ipld,
                Bucket &bucket1,
                Bucket &bucket2,
                Set &set1,
                Set &set2) {
    if (bucket1.shard && bucket2.shard) {
      return hamtDiffShard(ipld, bucket1, bucket2, set1, set2);
    }
    if (bucket2.shard) {
      return hamtDiff(ipld, bucket2, bucket1, set2, set1);
    }
    if (!hamtVisit(ipld, bucket1, set1)) {
      return false;
    }
    return true;
  }

  inline bool hamtDiff(const CbIpldPtr &ipld,
                       const CbCid &cid1,
                       const CbCid &cid2,
                       Set &set1,
                       Set &set2) {
    if (cid1 == cid2) {
      return true;
    }
    Bucket bucket1{&cid1}, bucket2{&cid2};
    return hamtDiffShard(ipld, bucket1, bucket2, set1, set2);
  }
}  // namespace fc::hamt_diff

namespace fc::diff {
  using codec::cbor::CborToken;
  using codec::cbor::readCborBlake;
  using codec::cbor::light_reader::HamtWalk;
  using primitives::ActorId;
  using primitives::BigInt;
  using primitives::Nonce;
  using primitives::RleBitset;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetKey;

  constexpr BytesN<1> kCborList0{0x80};
  const auto kStateRootInfo{CbCid::hash(kCborList0)};

  CbIpldPtr ipld;

  std::string X(size_t x) {
    return std::to_string((size_t)ceil(x / 1024.0)) + " KB";
  }
  std::string Xm(size_t x) {
    x = ceil(x / 1024.0);
    if (x < 1000) return std::to_string(x) + " KB";
    x = ceil(x / 1024.0);
    return std::to_string(x) + " MB";
  }
  std::string time(const Since &t) {
    return std::to_string(t.ms<size_t>()) + " ms";
  }

  const std::set<ActorCodeCid> __codes{
      vm::actor::builtin::v7::kAccountCodeId,
      vm::actor::builtin::v7::kCronCodeId,
      vm::actor::builtin::v7::kStoragePowerCodeId,
      vm::actor::builtin::v7::kStorageMarketCodeId,
      vm::actor::builtin::v7::kStorageMinerCodeId,
      vm::actor::builtin::v7::kMultisigCodeId,
      vm::actor::builtin::v7::kInitCodeId,
      vm::actor::builtin::v7::kPaymentChannelCodeId,
      vm::actor::builtin::v7::kRewardActorCodeId,
      vm::actor::builtin::v7::kSystemActorCodeId,
      vm::actor::builtin::v7::kVerifiedRegistryCodeId,
  };

  struct ActorX {
    ActorCodeCid code;
    CbCid head;
    Nonce nonce;
    BigInt balance;
    size_t _size{};

    ActorX(BytesIn input) {
      _size = input.size();
      CborToken token;
      assert(read(token, input).listCount() == 4);
      BytesIn _cid;
      assert(read(token, input).cidSize());
      assert(codec::read(_cid, input, *token.cidSize()));
      BytesIn _code;
      assert(codec::cbor::light_reader::readRawId(_code, _cid));
      assert(_cid.empty());
      code = ActorCodeCid{common::span::bytestr(_code)};
      auto it{__codes.find(code)};
      assert(it != __codes.end());
      code = *it;
      const CbCid *_head{};
      assert(readCborBlake(_head, input));
      head = *_head;
      assert(read(token, input).asUint());
      nonce = *token.asUint();
      assert(read(token, input).bytesSize());
      const auto _balance{*codec::read(input, *token.bytesSize())};
      if (!_balance.empty()) {
        assert(_balance[0] == 0);
        import_bits(balance, _balance.begin() + 1, _balance.end());
      }
    }
    size_t size() const {
      return _size - code.size();
    }
  };

  size_t sizeBytes(BytesIn x) {
    Bytes buf;
    codec::cbor::writeBytes(buf, x.size());
    return buf.size() + x.size();
  }
  size_t sizeRle(const RleBitset &bits) {
    return sizeBytes(codec::rle::encode(bits));
  }

  CbCid f_ts(const TipsetCPtr &ts) {
    Bytes value;
    assert(ipld->get(*asBlake(ts->getParentStateRoot()), value));
    BytesIn input{value};
    CborToken token;
    assert(read(token, input).listCount() == 3);
    assert(read(token, input).asUint() == 4);
    const CbCid *hamt{};
    assert(readCborBlake(hamt, input));
    const CbCid *info{};
    assert(readCborBlake(info, input));
    assert(*info == kStateRootInfo);
    return *hamt;
  }
  void f_car(const std::string &path, std::set<size_t> steps) {
    if (!std::filesystem::exists(path)) return;
    fmt::print("\n");
    fmt::print("car {}\n", path);
    ipld =
        *storage::cids_index::loadOrCreateWithProgress(path, false, {}, {}, {});
    const auto head_tsk{
        *TipsetKey::make(storage::car::readHeader(path).value())};
    const auto _ipld{std::make_shared<CbAsAnyIpld>(ipld)};
    primitives::tipset::TsLoadIpld load{_ipld};
    const auto head{load.load(head_tsk).value()};
    const auto state2{f_ts(head)};
    {
      size_t objects2{}, actors2{};
      RleBitset ids2;
      HamtWalk hamt{ipld, state2};
      BytesIn _addr, _actor;
      while (hamt.next(_addr, _actor)) {
        ActorId id{};
        assert(codec::cbor::light_reader::readIdAddress(id, _addr));
        ids2.insert(id);
        ActorX actor{_actor};
        actors2 += actor.size();
      }
      Bytes buf;
      for (const auto &cid : hamt.cids) {
        assert(ipld->get(cid, buf));
        objects2 += buf.size();
      }
      const auto n{hamt.cids.size()};
      fmt::print("  Actors HAMT: {} objects ({}), {} actors ({})\n",
                 n,
                 Xm(sizeof(CbCid) * n + objects2),
                 ids2.size(),
                 Xm(sizeRle(ids2) + actors2));
    }
    for (const auto &_step : steps) {
      auto ts{head};
      auto step{_step};
      while (ts->height() != 0 && step--) {
        ts = load.load(ts->getParents()).value();
      }
      const auto state1{f_ts(ts)};
      fmt::print("  {} epochs\n", _step == SIZE_MAX ? SIZE_MAX - _step : _step);
      {
        Since t;
        const auto f{[&](const CbCid &cid) {
          std::map<CbCid, size_t> m;
          HamtWalk hamt{ipld, cid};
          BytesIn _addr, _actor;
          while (hamt.next(_addr, _actor)) {
            // ignore
          }
          Bytes buf;
          for (const auto &cid : hamt.cids) {
            assert(ipld->get(cid, buf));
            m.emplace(cid, buf.size());
          }
          return m;
        }};
        const auto objects1{f(state1)}, objects2{f(state2)};
        size_t size{}, count{};
        for (const auto &[k, v] : objects2) {
          if (!objects1.count(k)) {
            size += v;
            ++count;
          }
        }
        fmt::print("    {} changed objects\n", count);
        fmt::print("    objects: {}, {}\n", X(size), time(t));
        {
          Since t;
          hamt_diff::Set set1, set2;
          assert(hamt_diff::hamtDiff(ipld, state1, state2, set1, set2));
          size_t check{};
          for (const auto &c : set2) {
            if (objects1.count(c)) continue;
            ++check;
          }
          assert(check == count);
          const auto _t{time(t)};
          for (const auto &c : set1) {
            assert(objects1.count(c));
          }
          for (const auto &c : set2) {
            assert(objects2.count(c));
          }
          for (const auto &[k, v] : objects2) {
            if (objects1.count(k)) continue;
            assert(!set1.count(k));
            assert(set2.count(k));
          }
          fmt::print("      {}\n", _t);
        }
      }
      {
        Since t;
        hamt_diff::RleMapDiff rle;
        assert(hamt_diff::hamtDiff(
            ipld, state1, state2, hamt_diff::cbVarint<true>(std::ref(rle))));
        fmt::print("    {} changed actors\n",
                   rle.remove_keys.size() + rle.add_keys.size()
                       + rle.change_keys.size());
        size_t size{};
        size += sizeRle(rle.remove_keys);
        size += sizeRle(rle.add_keys);
        size += sizeRle(rle.change_keys);
        for (const auto &[_id, _actor] : rle.add) {
          ActorX actor{_actor};
          size += actor.size();
        }
        for (const auto &[_id, _actor12] : rle.change) {
          const auto &[_actor1, _actor2]{_actor12};
          ActorX actor2{_actor2};
          size += actor2.size();
        }
        fmt::print("    actors: {}, {}\n", X(size), time(t));
      }
      {
        Since t;
        hamt_diff::RleMapDiff rle;
        assert(hamt_diff::hamtDiff(
            ipld, state1, state2, hamt_diff::cbVarint<true>(std::ref(rle))));
        size_t size{};
        size += sizeRle(rle.remove_keys);
        size += sizeRle(rle.add_keys);
        size += sizeRle(rle.change_keys);
        for (const auto &[_id, _actor] : rle.add) {
          ActorX actor{_actor};
          size += actor.size();
        }
        for (const auto &[_id, _actor12] : rle.change) {
          const auto &[_actor1, _actor2]{_actor12};
          ActorX actor1{_actor1}, actor2{_actor2};
          const auto nonce{actor2.nonce - actor1.nonce};
          const BigInt balance{actor2.balance - actor1.balance};
          if (nonce) {
            size += 1 + CborToken::_more(nonce);
          }
          if (balance) {
            struct It {
              size_t n{};

              auto &operator*() {
                return *this;
              }
              void operator=(uint8_t) {}
              void operator++() {
                ++n;
              }
            };
            const auto n{export_bits(balance, It{}, 8).n};
            size += 1 + CborToken::_more(n) + n;
          }
          if (actor2.head != actor1.head) {
            size += sizeof(CbCid);
          }
        }
        size += rle.change_keys.size() * 3 / 8;
        fmt::print("    actors bits: {}, {}\n", X(size), time(t));
      }
    }
  }

  extern "C" int main() {
    // https://fil-chain-snapshots-fallback.s3.amazonaws.com/mainnet/minimal_finality_stateroots_1675200_2022-03-29_14-00-00.car
    f_car("/data/x/data/mainnet-1675200.car", {100, 800, 1600});
    return 0;
  }
}  // namespace fc::diff
