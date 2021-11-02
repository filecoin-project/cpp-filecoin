/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fwd.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/leveldb/prefix.hpp"
#include "vm/version/version.hpp"

#include <mutex>

namespace fc::api {
  struct MsgWait;
  struct AddChannelInfo;
}  // namespace fc::api

namespace fc::paych_maker {
  using ApiPtr = std::shared_ptr<api::FullNodeApi>;
  using api::AddChannelInfo;
  using api::MsgWait;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using storage::MapPtr;
  using storage::OneKey;
  using vm::message::SignedMessage;
  using vm::version::NetworkVersion;

  using FromTo = std::pair<Address, Address>;

  using Cb = std::function<void(outcome::result<AddChannelInfo>)>;

  struct Row {
    static Bytes key(const FromTo &from_to);

    boost::optional<Address> actor;
    TokenAmount total_amount;
    TokenAmount unused_amount;
    boost::optional<CID> waiting_cid;
    boost::optional<TokenAmount> waiting_amount;
  };
  CBOR_TUPLE(
      Row, actor, total_amount, unused_amount, waiting_cid, waiting_amount)

  struct Queue {
    using Map = std::map<FromTo, Queue>;

    void save();

    OneKey key;
    Row row{};
    std::vector<Cb> waiting_cb{};
    TokenAmount pending_amount{};
    std::vector<Cb> pending_cb{};
  };

  struct UnusedCid {
    const CID &get();
    void set(const CID &new_cid);

    OneKey key;
    boost::optional<CID> cid{};
  };

  struct PaychMaker {
    using It = Queue::Map::iterator;

    PaychMaker(const ApiPtr &api, const MapPtr &kv);

    void make(const FromTo &from_to, const TokenAmount &amount, Cb cb);

    void onNetwork(It it, outcome::result<NetworkVersion> _network);
    void onPush(It it, outcome::result<SignedMessage> _smsg);
    void onWait(It it, outcome::result<MsgWait> _wait);
    void onError(It it, std::error_code ec);
    void next(It it);

    ApiPtr api;
    MapPtr kv;
    UnusedCid unused_cid;
    std::mutex mutex;
    Queue::Map map;
  };
}  // namespace fc::paych_maker
