/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "cli/node/node.hpp"
#include "cli/validate/address.hpp"
#include "cli/validate/cid.hpp"

namespace fc::cli::cli_node {
  using api::BlockMessages;
  using api::CidMessage;
  using api::MessageReceipt;
  using api::MinerPower;
  using api::SignedMessage;

  std::string EpochTime(ChainEpoch current, ChainEpoch start) {
    if (current > start)
      return fmt::format("{} ({} ago)",
                         fmt::to_string(start),
                         fmt::to_string(current - start));
    if (current == start) 
      return fmt::format("{} (now)", fmt::to_string(start));
    if (current < start)
      return fmt::format(
          "{} (in {})", fmt::to_string(start), fmt::to_string(start - current));
  }

  struct Node_developer_pending {
    struct Args {
      CLI_BOOL("local", "output will consist of local messages") local;
      CLI_BOOL("cids", "only print cids of messages in output") cids;
      CLI_OPTIONAL("to",
                   "returns only messagrs addressed to the given address",
                   Address)
      to;
      CLI_OPTIONAL("from", "return messages from a given address", Address)
      from;

      CLI_OPTS() {
        Opts opts;
        local(opts);
        cids(opts);
        to(opts);
        from(opts);
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};

      std::set<Address> filter{};
      if (args.local) {
        auto addresses = cliTry(api->WalletList());
        for (const auto &address : addresses) {
          filter.insert(address);
        }
      }
      auto messages = cliTry(api->MpoolPending(TipsetKey()));
      for (const auto &message : messages) {
        if (args.local && filter.find(message.message.from) == filter.end())
          continue;
        if (args.from && message.message.from != *args.from) continue;
        if (args.to && message.message.to != *args.to) continue;
        if (args.cids) {
          fmt::print(fmt::to_string(message.getCid()));
        } else {
          auto jsoned = api::encode(message);
          auto bytes_json = cliTry(codec::json::format(&jsoned));
          auto response = common::span::bytestr(bytes_json);
          fmt::print(response);
        }
      }
    }
  };

  struct Node_developer_sub {
    // TODO: Implement it
  };

  struct Node_developer_find {
    struct Args {
      CLI_OPTIONAL("from",
                   "search for messages with given 'from' address",
                   Address)
      from;
      CLI_OPTIONAL("to", "search for messages with given 'to' address", Address)
      to;
      CLI_OPTIONAL("method", "search for messages with given method", uint64_t)
      method;

      CLI_OPTS() {
        Opts opts;
        from(opts);
        to(opts);
        method(opts);
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
      auto pending = cliTry(api->MpoolPending(TipsetKey()));
      std::vector<api::SignedMessage> out;
      for (const auto &message : pending) {
        if (args.from && message.message.from != *args.from) continue;
        if (args.to && message.message.to != *args.to) continue;
        if (args.method && message.message.method != *args.method) continue;
        out.push_back(message);
      }
      auto jsoned = api::encode(out);
      auto bytes_json = cliTry(codec::json::format(&jsoned));
      auto response = common::span::bytestr(bytes_json);
      fmt::print(response);
    }
  };

  struct Node_developer_createToken {
    struct Args {
      CLI_OPTIONAL(
          "perm",
          "permission to assign to the token, one of: read, write, sign, admin",
          std::string_view)
      perm;
      // TODO: Permission decode;
      CLI_OPTS() {
        Opts opts;
        perm(opts);
        return opts;
      }
    };

    CLI_RUN() {
      // TODO: comtinue method
    }
  };

  struct Node_developer_apiInfo {
    // TODO: develop
  };

  struct Node_developer_head : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      auto head = cliTry(api->ChainHead());
      for (const auto &cid : head->key.cids()) {
        fmt::print(fmt::to_string(cid));
      }
    }
  };

  struct Node_developer_getBlock {
    struct Args {
      CLI_BOOL("raw", "Get a block and print its details") raw;

      CLI_OPTS() {
        Opts opts;
        raw(opts);
        return opts;
      };
    };
    CLI_RUN() {
      auto block_cid{cliArgv<CID>(argv, 0, "block CID")};
      Node::Api api{argm};
      auto block = cliTry(api->ChainGetBlock(block_cid), "get block failed");
      if (args.raw) {
        auto jsoned = api::encode(block);
        auto bytes_json = cliTry(codec::json::format(&jsoned));
        auto response = common::span::bytestr(bytes_json);
        fmt::print(response);
      } else {
        const BlockMessages messages = cliTry(
            api->ChainGetBlockMessages(block_cid), "failed to get messages");
        const std::vector<CidMessage> parent_messages =
            cliTry(api->ChainGetParentMessages(block_cid),
                   "failed to get parent messages");
        const std::vector<MessageReceipt> receipts = cliTry(
            api->ChainGetParentReceipts(block_cid), "failed to get receipts");
        // TODO: Continue function;
      }
    }
  };

  struct Node_developer_readObject : Empty {
    CLI_RUN() {
      auto object_cid{cliArgv<CID>(argv, 0, "object CID")};
      Node::Api api{argm};
      Bytes object = cliTry(api->ChainReadObj(object_cid));
      fmt::print("{}", object);
    }
  };

  struct Node_developer_getMessage : Empty {
    CLI_RUN() {
      auto message_cid{cliArgv<CID>(argv, 0, "message CID")};
      Node::Api api{argm};
      std::shared_ptr<ApiIpfsDatastore> ipfs =
          std::make_shared<ApiIpfsDatastore>(api.api);
      SignedMessage message =
          cliTry(fc::getCbor<SignedMessage>(ipfs, message_cid));
      auto jsoned = api::encode(message);
      auto bytes_json = cliTry(codec::json::format(&jsoned));
      auto response = common::span::bytestr(bytes_json);
      fmt::print(response);
    }
  };

  struct Node_developer_get {
    // TODO: develop
  };

  struct Node_developer_slashConsensus {
    // TODO: develop
  };

  struct Node_developer_estimateGasPrices : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      std::vector<uint64_t> number_of_blocks{1, 2, 3, 5, 10, 20, 50, 100, 300};
      for (const auto &number : number_of_blocks) {
        TokenAmount result = cliTry(api->GasEstimateGasPremium(
            number, vm::actor::kSystemActorAddress, 10000, TipsetKey()));
        fmt::print("For {} blocks: {} FIL",
                   fmt::to_string(number),
                   fmt::to_string(result));
      }
    }
  };

  struct Node_developer_stateMinerInfo : Empty {
    CLI_RUN() {
      auto miner_address{cliArgv<Address>(argv, 0, "miner Address")};
      Node::Api api{argm};
      auto head = cliTry(api->ChainHead());
      api::MinerInfo miner_info =
          cliTry(api->StateMinerInfo(miner_address, head->key));
      auto availableBalance =
          cliTry(api->StateMinerAvailableBalance(miner_address, head->key));

      fmt::print("Available balance: {}\nOwner: {}\nWorker: {}",
                 fmt::to_string(availableBalance),
                 fmt::to_string(miner_info.owner),
                 fmt::to_string(miner_info.worker));

      for (const auto &controlAddress : miner_info.control) {
        fmt::print("Control: \t{}\n", fmt::to_string(controlAddress));
      }
      fmt::print("PeerID: \t{}\n", miner_info.peer_id);
      fmt::print("MultiAddresses:\t");
      for (const auto &multiaddr : miner_info.multiaddrs) {
        fmt::print("{} ", fmt::to_string(multiaddr));
      }
      fmt::print("\n");
      fmt::print("Consensus Fault End: \t{}\n",
                 "None");  // TODO: Consensus fault
      fmt::print("Sector Size: \t{}\n", fmt::to_string(miner_info.sector_size));
      MinerPower power = cliTry(api->StateMinerPower(miner_address, head->key));

      fmt::print("Byte Power: {}/{}\t{}%\n",
                 fmt::to_string(power.miner.raw_power),
                 fmt::to_string(power.total.raw_power),
                 fmt::to_string(bigdiv({power.miner.raw_power * 100},
                                       power.total.raw_power)));

      fmt::print("Actual power: {}/{}\t{}%\n",
                 fmt::to_string(power.miner.qa_power),
                 fmt::to_string(power.total.qa_power),
                 fmt::to_string(bigdiv({power.miner.qa_power * 100},
                                       power.total.qa_power)));

      auto proving_deadline =
          cliTry(api->StateMinerProvingDeadline(miner_address, head->key));

      fmt::print("Proving period start:\t{}\n",
                 EpochTime(proving_deadline.current_epoch,
                           proving_deadline.period_start));
    }
  };

}  // namespace fc::cli::cli_node