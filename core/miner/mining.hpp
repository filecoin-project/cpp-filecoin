/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/functional/hash.hpp>
#include <unordered_set>

#include "api/api.hpp"
#include "sector_storage/spec_interfaces/prover.hpp"

namespace fc::mining {
  using api::Address;
  using api::Api;
  using api::BigInt;
  using api::BlockTemplate;
  using api::SignedMessage;
  using api::Tipset;
  using api::TipsetKey;
  using boost::asio::io_context;
  using boost::asio::system_timer;
  using sector_storage::Prover;

  struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &pair) const {
      return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
  };

  struct Mining : std::enable_shared_from_this<Mining> {
    static constexpr std::chrono::seconds kBlockDelay{25};
    static constexpr std::chrono::seconds kPropagationDelay{6};
    static std::shared_ptr<Mining> create(std::shared_ptr<io_context> io,
                                          std::shared_ptr<Api> api,
                                          std::shared_ptr<Prover> prover,
                                          const Address &miner);
    void start();
    void waitParent();
    outcome::result<void> prepare();
    outcome::result<void> submit(BlockTemplate block1);
    outcome::result<void> bestParent();
    template <typename F>
    void asyncWait(F f) {
      timer->async_wait([f{std::move(f)}](auto ec) {
        if (!ec) {
          f();
        }
      });
    }
    std::shared_ptr<io_context> io;
    std::unique_ptr<system_timer> timer;
    std::shared_ptr<Api> api;
    std::shared_ptr<Prover> prover;
    Address miner;
    boost::optional<Tipset> ts;
    BigInt weight;
    size_t skip{};
    std::unordered_set<std::pair<TipsetKey, size_t>, pair_hash> mined;
  };

  bool isTicketWinner(BytesIn ticket,
                      const BigInt &power,
                      const BigInt &total_power);

  outcome::result<boost::optional<BlockTemplate>> prepareBlock(
      const Address &miner,
      const Tipset &ts,
      uint64_t height,
      std::shared_ptr<Api> api,
      std::shared_ptr<Prover> prover);

  outcome::result<std::vector<SignedMessage>> selectMessages(
      std::shared_ptr<Api> api, const Tipset &ts);
}  // namespace fc::mining
