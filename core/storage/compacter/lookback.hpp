/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/light_reader/actor.hpp"
#include "codec/cbor/light_reader/address.hpp"
#include "codec/cbor/light_reader/amt_walk.hpp"
#include "codec/cbor/light_reader/hamt_walk.hpp"
#include "codec/cbor/light_reader/miner_actor_reader.hpp"
#include "codec/cbor/light_reader/state_tree.hpp"
#include "codec/cbor/light_reader/storage_power_actor_reader.hpp"
#include "common/append.hpp"
#include "storage/compacter/queue.hpp"
#include "vm/actor/codes.hpp"

#define ACTOR_CODE_IS(name)                                                    \
  code == vm::actor::builtin::v0::name || code == vm::actor::builtin::v2::name \
      || code == vm::actor::builtin::v3::name                                  \
      || code == vm::actor::builtin::v4::name                                  \
      || code == vm::actor::builtin::v5::name                                  \
      || code == vm::actor::builtin::v6::name

// TODO(turuslan): keep own miner sectors
namespace fc::storage::compacter {
  inline void lookbackActor(std::vector<CbCid> &copy,
                            std::vector<CbCid> &recurse,
                            const CbIpldPtr &ipld,
                            const ActorCodeCid &code,
                            const CbCid &head) {
    if (ACTOR_CODE_IS(kAccountCodeId)) {
      copy.push_back(head);
      return;
    }
    if (ACTOR_CODE_IS(kInitCodeId)) {
      recurse.push_back(head);
      return;
    }
    if (ACTOR_CODE_IS(kStorageMinerCodeId)) {
      const auto v0{code == vm::actor::builtin::v0::kStorageMinerCodeId};
      const auto _r{
          codec::cbor::light_reader::readMinerActorInfo(ipld, head, v0)};
      if (!_r) {
        return;
      }
      const auto &[info, sectors, deadlines]{_r.value()};
      copy.push_back(head);
      copy.push_back(info);
      recurse.push_back(sectors);
      copy.push_back(deadlines);
      codec::cbor::light_reader::minerDeadlines(
          ipld, deadlines, [&](const CbCid &deadline, const CbCid &partitions) {
            copy.push_back(deadline);
            codec::cbor::light_reader::AmtWalk walk{ipld, partitions};
            if (!walk.visit()) {
              return false;
            }
            append(copy, walk.cids);
            return true;
          });
      return;
    }
    if (ACTOR_CODE_IS(kStoragePowerCodeId)) {
      const auto v0{code == vm::actor::builtin::v0::kStoragePowerCodeId};
      const auto _r{codec::cbor::light_reader::readStoragePowerActorClaims(
          ipld, head, v0)};
      if (!_r) {
        return;
      }
      const auto &claims{_r.value()};
      copy.push_back(head);
      recurse.push_back(claims);
      return;
    }
  }

  inline void lookbackActors(std::vector<CbCid> &copy,
                             std::vector<CbCid> &recurse,
                             const CbIpldPtr &ipld,
                             const CbIpldPtr &visited,
                             const CbCid &state) {
    CbCid _hamt;
    if (!codec::cbor::light_reader::readStateTree(_hamt, ipld, state)) {
      return;
    }
    copy.push_back(state);
    copy.push_back(_hamt);
    codec::cbor::light_reader::HamtWalk hamt{ipld, _hamt};
    hamt.visited = visited;
    BytesIn _addr;
    BytesIn _actor;
    while (hamt.next(_addr, _actor)) {
      uint64_t id = 0;
      ActorCodeCid code;
      const CbCid *head = nullptr;
      if (!codec::cbor::light_reader::readIdAddress(id, _addr)) {
        throw std::logic_error{"lookbackActors readId"};
      }
      if (!codec::cbor::light_reader::readActor(code, head, _actor)) {
        throw std::logic_error{"lookbackActors readActor"};
      }
      if (!visited->has(*head)) {
        lookbackActor(copy, recurse, ipld, code, *head);
      }
    }
    copy.insert(copy.begin(), hamt.cids.begin(), hamt.cids.end());
  }
}  // namespace fc::storage::compacter

#undef ACTOR_CODE_IS
