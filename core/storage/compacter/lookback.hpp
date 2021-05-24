/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/light_reader/actor.hpp"
#include "codec/cbor/light_reader/address.hpp"
#include "codec/cbor/light_reader/hamt_walk.hpp"
#include "codec/cbor/light_reader/miner_actor_reader.hpp"
#include "codec/cbor/light_reader/state_tree.hpp"
#include "codec/cbor/light_reader/storage_power_actor_reader.hpp"
#include "storage/compacter/queue.hpp"
#include "vm/actor/codes.hpp"

// TODO: keep own miner sectors
namespace fc::storage::compacter {
  using vm::actor::code::Code;

  constexpr bool anyOf(
      Code code, Code code0, Code code2, Code code3, Code code4) {
    return code == code0 || code == code2 || code == code3 || code == code4;
  }

  inline void lookbackActor(std::vector<CbCid> &copy,
                            std::vector<CbCid> &recurse,
                            const CbIpldPtr &ipld,
                            std::string_view code,
                            const CbCid &head) {
    using namespace vm::actor::code;
    using vm::actor::ActorVersion;
    if (anyOf(code, account0, account2, account3, account4)) {
      copy.push_back(head);
      return;
    }
    if (anyOf(code, init0, init2, init3, init4)) {
      recurse.push_back(head);
      return;
    }
    if (anyOf(code, miner0, miner2, miner3, miner4)) {
      auto _r{codec::cbor::light_reader::readMinerActorInfo(
          ipld, head, ActorVersion{code == miner0 ? 0 : 1})};
      if (!_r) {
        return;
      }
      auto &[info, sectors, deadlines]{_r.value()};
      copy.push_back(head);
      copy.push_back(info);
      return;
    }
    if (anyOf(code, power0, power2, power3, power4)) {
      auto _r{codec::cbor::light_reader::readStoragePowerActorClaims(
          ipld, head, ActorVersion{code == power0 ? 0 : 1})};
      if (!_r) {
        return;
      }
      auto &claims{_r.value()};
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
    BytesIn _addr, _actor;
    while (hamt.next(_addr, _actor)) {
      uint64_t id;
      std::string_view code;
      const CbCid *head;
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
    return;
  }
}  // namespace fc::storage::compacter
