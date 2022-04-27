/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/algorithm/string.hpp>
#include "api/full_node/node_api.hpp"
#include "cli/node/node.hpp"
#include "cli/validate/address.hpp"
#include "cli/validate/cid.hpp"
#include "codec/uvarint.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::cli::cli_node {
  using api::BlockMessages;
  using api::CidMessage;
  using api::MinerInfo;
  using api::MinerPower;
  using api::MsgWait;
  using api::SignedMessage;
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using storage::ipfs::ApiIpfsDatastore;
  using vm::VMExitCode;
  using vm::actor::Actor;
  using vm::actor::MethodParams;
  using vm::actor::builtin::types::miner::SectorOnChainInfo;
  using vm::actor::builtin::types::storage_power::Claim;
  using vm::message::UnsignedMessage;
  using vm::runtime::MessageReceipt;
  using base64 = cppcodec::base64_rfc4648;
  using common::unhex;
  using primitives::DealId;

  using tipset_template =
      CLI_OPTIONAL("tipset,t",
                   "specify tipset to call method on (pass comma separated "
                   "array of cids)",
                   std::string);

  TipsetCPtr loadTipset(const Node::Api &api,
                        const boost::optional<std::string> &tipset_key_str) {
    if (not tipset_key_str) {
      return cliTry(api->ChainHead());
    }
    if (tipset_key_str.value()[0] == '@') {
      if (tipset_key_str.value() == "@head") {
        return cliTry(api->ChainHead());
      }

      return cliTry(api->ChainGetTipSetByHeight(
          std::stoi(tipset_key_str.value().substr(1, std::string::npos)), {}));
    }
    std::vector<std::string> string_cids;
    boost::split(string_cids, tipset_key_str.value(), boost::is_any_of(","));

    std::vector<CID> cids(string_cids.size());
    std::transform(string_cids.begin(),
                   string_cids.end(),
                   cids.begin(),
                   [](const std::string &string_cid) {
                     return CID::fromString(string_cid).value();
                   });

    return cliTry(api->ChainGetTipSet(TipsetKey::make(cids).value()));
  }

  std::string makeKib(const primitives::BigInt &x) {
    return fmt::format("{} KiB",
                       x / 1024);  // TODO is correct or cast to double?
  }

  std::string makeFil(const primitives::BigInt &x) {
    return fmt::format(
        "{} FIL",
        x / kFilecoinPrecision);  // TODO is correct or cast to double?
  }

  std::string epochTime(ChainEpoch current, ChainEpoch start) {
    if (current > start)
      return fmt::format("{} ({} ago)",
                         fmt::to_string(start),
                         fmt::to_string(current - start));
    if (current == start) return fmt::format("{} (now)", fmt::to_string(start));
    if (current < start)
      return fmt::format(
          "{} (in {})", fmt::to_string(start), fmt::to_string(start - current));
  }

  void printReceiptReturn(const Node::Api &api,
                          const UnsignedMessage &message,
                          const MessageReceipt &message_receipt) {
    if (message_receipt.exit_code == VMExitCode::kOk) {
      return;
    }
    const Actor actor = cliTry(api->StateGetActor(message.to, TipsetKey{}));
    //    TODO
    //    auto jret =
    //        jsonReturn(actor.code, message.method,
    //        message_receipt.return_value);
    //
    //    fmt::print("Decoded return value: \n", jret);
  }

  void printMessage(const Node::Api &api,
                    const CID &message_cid,
                    const MsgWait &message_wait,
                    const UnsignedMessage &message) {
    if (message_wait.message != message_cid) {
      fmt::print("Message was replaced: {}\n", message_wait.message);
    }

    fmt::print("Executed in tipset: ");
    const std::vector<CbCid> CbCids = message_wait.tipset.cids();
    for (auto it = CbCids.begin(); it != --CbCids.end(); ++it) {
      fmt::print("{},", CID(*it).toString().value());
    }
    fmt::print("{}\n", CID(CbCids.back()).toString().value());
    fmt::print("Exit Code: {}\n", message_wait.receipt.exit_code);
    fmt::print("Gas Used: {}\n", message_wait.receipt.gas_used);
    fmt::print("Return: {}\n\n",
               common::hex_lower(message_wait.receipt.return_value));
    printReceiptReturn(api, message, message_wait.receipt);
  }

  struct Node_mpool_pending {
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

  struct Node_mpool_sub : Empty {
    // TODO: Implement it
    CLI_RUN() {}
  };

  struct Node_mpool_find {
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

  struct Node_auth_createToken {
    struct Args {
      CLI_OPTIONAL(
          "perm",
          "permission to assign to the token, one of: read, write, sign, admin",
          std::string)
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

  struct Node_auth_apiInfo : Empty {
    // TODO: develop
    CLI_RUN() {}
  };

  struct Node_chain_head : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      auto head = cliTry(api->ChainHead());
      for (const auto &cid : head->key.cids()) {
        fmt::print("{},", CID(cid).toString().value());
      }
      fmt::print("\n");
    }
  };

  struct Node_chain_getBlock {
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

  struct Node_chain_readObject : Empty {
    CLI_RUN() {
      auto object_cid{cliArgv<CID>(argv, 0, "object CID")};
      Node::Api api{argm};
      Bytes object = cliTry(api->ChainReadObj(object_cid));
      fmt::print("{}\n", common::hex_lower(object));
    }
  };

  struct Node_chain_getMessage : Empty {
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

  struct Node_chain_get : Empty {
    // TODO: develop
    CLI_RUN() {}
  };

  struct Node_chain_slashConsensus : Empty {
    // TODO: develop
    CLI_RUN() {}
  };

  struct Node_chain_estimateGasPrices : Empty {
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

  struct Node_state_stateMinerInfo : Empty {
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
      fmt::print("PeerID: \t{}\n", common::hex_lower(miner_info.peer_id));
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
                 epochTime(proving_deadline.current_epoch,
                           proving_deadline.period_start));
    }
  };

  struct Node_state_networkVersion : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const NetworkVersion network_version = cliTry(
          api->StateNetworkVersion(TipsetKey()), "Getting Chain Version...");

      fmt::print("Network version: {}\n", network_version);
    }
  };

  struct Node_state_market_balance : public Node_client_balances {
  };  // TODO wrong

  struct Node_state_sector {
    struct Args {
      tipset_template tipset;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const Address miner_address{cliArgv<Address>(argv, 0, "Miner address")};
      const SectorNumber sector_number{
          cliArgv<SectorNumber>(argv, 1, "Sector number")};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const std::string_view process_desc = "Get info from sector...";
      const SectorOnChainInfo sector_info =
          cliTry(cliTry(api->StateSectorGetInfo(
                            miner_address, sector_number, tipset->key),
                        process_desc),
                 process_desc);

      fmt::print("SectorNumber: {}\n", sector_info.sector);
      fmt::print("SealProof: {}\n", sector_info.seal_proof);
      fmt::print("SealedCID: {}\n", sector_info.sealed_cid);
      if (sector_info.sector_key_cid) {
        fmt::print("SectorKeyCID: {}\n", sector_info.sector_key_cid.value());
      }
      fmt::print("DealIDs: {}\n\n", fmt::join(sector_info.deals, ", "));

      fmt::print("Activation: {}\n",
                 epochTime(tipset->height(), sector_info.activation_epoch));
      fmt::print("Expiration: {}\n\n",
                 epochTime(tipset->height(), sector_info.expiration));

      fmt::print("DealWeight: {}\n", sector_info.deal_weight);
      fmt::print("VerifiedDealWeight: {}\n", sector_info.verified_deal_weight);
      fmt::print("InitialPledge: {}\n", makeFil(sector_info.init_pledge));
      fmt::print("ExpectedDayReward: {}\n",
                 makeFil(sector_info.expected_day_reward));
      fmt::print("ExpectedStoragePledge: {}\n\n",
                 makeFil(sector_info.expected_storage_pledge));

      const api::SectorLocation sector_partition = cliTry(
          api->StateSectorPartition(miner_address, sector_number, tipset->key),
          "Getting sector partition...");

      fmt::print("Deadline: {}\n", sector_partition.deadline);
      fmt::print("Partition: {}\n", sector_partition.partition);
    }
  };

  struct Node_state_call {
    struct Args {
      tipset_template tipset;
      CLI_DEFAULT("from",
                  "Address from",
                  Address,
                  {vm::actor::kSystemActorAddress})
      from;
      CLI_DEFAULT("value",
                  "specify value field for invocation",
                  int,
                  {0})  // TODO parse FIL ?
      value;
      CLI_DEFAULT("ret",
                  "specify how to parse output (raw, decoded, base64, hex). "
                  "Default: decoded",
                  std::string,
                  {"decoded"})
      ret;
      CLI_DEFAULT(
          "encoding",
          "specify params encoding to parse (base64, hex). Default: base64",
          std::string,
          {"base64"})
      encoding;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        from(opts);
        value(opts);
        ret(opts);
        encoding(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};
      const Address actor_address{cliArgv<Address>(argv, 0, "Address actor")};
      const int method{cliArgv<int>(argv, 1, "Method to invoke")};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const MethodParams params = [&]() {
        if (argv.size() > 2) {  // TODO change ?
          const std::string params_string{
              cliArgv<std::string>(argv, 2, "Encode data")};
          if (*args.encoding == "base64") {
            return base64::decode(params_string);
          }
          if (*args.encoding == "hex") {
            return cliTry(unhex(params_string));
          }
          throw CliError("unrecognized encoding: {}", *args.encoding);
        }
        return MethodParams{};
      }();

      const api::InvocResult ret =
          cliTry(api->StateCall(vm::message::UnsignedMessage(actor_address,
                                                             *args.from,
                                                             {},
                                                             *args.value,
                                                             {},
                                                             {},
                                                             method,
                                                             params),
                                tipset->key));

      if (not ret.error.empty()) {
        throw CliError("invocation failed (exit: %d, gasUsed: %d): %s",
                       ret.receipt.exit_code,
                       ret.receipt.gas_used,
                       ret.error);
      }

      fmt::print("Call receipt:\n");
      fmt::print("Exit code: {}\n", ret.receipt.exit_code);
      fmt::print("Gas Used: {}\n", ret.receipt.gas_used);

      if (*args.ret == "decoded") {
        // TODO fill
      }
      if (*args.ret == "raw") {
        // TODO fill
      }
      if (*args.ret == "hex") {
        // TODO fill
      }
      if (*args.ret == "base64") {
        // TODO fill
      }
    }
  };

  struct Node_state_searchMsg : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const CID message_cid{
          cliArgv<CID>(argv, 0, "Message CID")};  // TODO Maybe decode?

      const std::string_view process_desc = "Getting state message...";
      const MsgWait message_wait = cliTry(
          cliTry(
              api->StateSearchMsg({}, message_cid, api::kLookbackNoLimit, true),
              process_desc),
          process_desc);

      const UnsignedMessage message =
          cliTry(api->ChainGetMessage(message_cid), "Chain get message...");

      printMessage(api, message_cid, message_wait, message);
    }
  };

  struct Node_state_waitMsg {
    struct Args {
      CLI_DEFAULT(
          "(NOT SUPPORT)"
          "timeout",
          "timeout default is 10m",
          std::string,
          {"10m"})
      timeout;

      CLI_OPTS() {
        Opts opts;
        timeout(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const CID message_cid{cliArgv<CID>(argv, 0, "Message CID")};

      const MsgWait message_wait = cliTry(api->StateWaitMsg(
          message_cid, kMessageConfidence, api::kLookbackNoLimit, true));

      const UnsignedMessage message = cliTry(api->ChainGetMessage(message_cid));

      printMessage(api, message_cid, message_wait, message);
    }
  };

  struct Node_state_sectorSize {
    struct Args {
      tipset_template tipset;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const Address miner_address{cliArgv<Address>(argv, 0, "Miner address")};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const MinerInfo miner_info =
          cliTry(api->StateMinerInfo(miner_address, tipset->key));

      fmt::print(
          "{} ({})\n", makeKib(miner_info.sector_size), miner_info.sector_size);
    }
  };

  struct Node_state_lookup {
    struct Args {
      tipset_template tipset;
      CLI_BOOL("reverse,r", "Perform reverse lookup") reverse;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        reverse(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const Address actor_address{cliArgv<Address>(argv, 0, "Actor address")};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const Address address_lookup = [&]() {
        if (not args.reverse) {
          return cliTry(api->StateLookupID(actor_address, tipset->key));
        }
        return cliTry(api->StateAccountKey(actor_address, tipset->key));
      }();

      fmt::print("{}\n", address_lookup);
    }
  };

  struct Node_state_getActor {
    struct Args {
      tipset_template tipset;
      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const Address actor_address{cliArgv<Address>(argv, 0, "Actor address")};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const Actor actor =
          cliTry(api->StateGetActor(actor_address, tipset->key));

      fmt::print("Address:\t{}\n", actor_address);
      fmt::print("Balance:\t{}\n", makeFil(actor.balance));
      fmt::print("Nonce:\t\t{}\n", actor.nonce);
      fmt::print("Code:\t\t{} ({})\n",
                 actor.code,
                 cliTry(actor.code.toString()));  // TODO matcher codeId
      fmt::print("Head:\t\t{}\n", actor.head);
    }
  };

  struct Node_state_listActors {
    struct Args {
      tipset_template tipset;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const std::vector<Address> addresses_of_actors =
          cliTry(api->StateListActors(tipset->key));

      for (const Address &address : addresses_of_actors) {
        fmt::print("{}\n", address);
      }
    }
  };

  struct Node_state_listMiners {
    struct Args {
      tipset_template tipset;
      CLI_DEFAULT("sort-by",
                  "criteria to sort miners by (none{default}, num-deals)",
                  std::string,
                  {"none"})
      sort_by;
      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        sort_by(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};
      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      std::vector<Address> miners = cliTry(api->StateListMiners(tipset->key));

      if (*args.sort_by == "num-deals") {
        const api::MarketDealMap all_deals = cliTry(api->StateMarketDeals({}));
        std::map<Address, int> out;
        for (const auto &[_, deal] : all_deals) {
          if (deal.state.sector_start_epoch != -1) {
            out[deal.proposal.provider]++;
          }
        }

        std::sort(miners.begin(),
                  miners.end(),
                  [&](const Address &lhs, const Address &rhs) {
                    return out[lhs] > out[rhs];
                  });

        for (int i = 0; i < std::min(50, (int)miners.size()); ++i) {
          fmt::print("{} {}\n", miners[i], out[miners[i]]);
        }

      } else if (*args.sort_by != "none") {
        throw CliError("unrecognized sorting order: {}\n", *args.sort_by);
      }

      fmt::print("{}\n", fmt::join(miners, "\n"));
    }
  };

  struct Node_state_getDeal : public Node_client_getDeal {
    struct Args {
      tipset_template tipset;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const DealId deal_id{cliArgv<DealId>(argv, 0, "Deal ID")};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const StorageDeal deal = cliTry(api->StateMarketStorageDeal(deal_id, tipset->key));

      fmt::print("{}\n", deal); // TODO json marshal?
    }
  };

  struct Node_state_activeSectors {
    struct Args {
      tipset_template tipset;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const Address miner_address{cliArgv<Address>(argv, 0, "Miner address")};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const std::vector<SectorOnChainInfo> sectors =
          cliTry(api->StateMinerActiveSectors(miner_address, tipset->key));

      for (const SectorOnChainInfo &sector : sectors) {
        fmt::print("{}: {}\n", sector.sector, sector.sealed_cid);
      }
    }
  };

  struct Node_state_sectors {
    struct Args {
      tipset_template tipset;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const Address miner_address{cliArgv<Address>(argv, 0, "Miner address")};

      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      const std::vector<SectorOnChainInfo> sectors =
          cliTry(api->StateMinerSectors(miner_address, {}, tipset->key));

      for (const SectorOnChainInfo &sector : sectors) {
        fmt::print("{}: {}\n", sector.sector, sector.sealed_cid);
      }
    }
  };

  struct Node_state_power {
    struct Args {
      tipset_template tipset;

      CLI_OPTS() {
        Opts opts;
        tipset(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};
      const TipsetCPtr tipset = loadTipset(api, args.tipset.v);

      Address miner_address{};
      const bool argv_empty = argv.empty();
      if (not argv_empty) {
        miner_address = cliArgv<Address>(argv, 0, "Miner address");

        const Actor actor =
            cliTry(api->StateGetActor(miner_address, tipset->key));
        auto version = cliTry(api->StateNetworkVersion(TipsetKey()),
                              "Getting Chain Version...");
        const auto address_matcher =
            vm::toolchain::Toolchain::createAddressMatcher(
                actorVersion(version));

        if (not address_matcher->isStorageMinerActor(actor.code)) {
          throw CliError(
              "provided address does not correspond to a miner actor");
        }
      }

      const api::MinerPower power =
          cliTry(api->StateMinerPower(miner_address, tipset->key));

      const Claim total_power = power.total;
      if (not argv_empty) {
        const Claim miner_power = power.miner;
        fmt::print("{}({}) / {}({}) ~= {}%\n",
                   miner_power.qa_power,
                   makeKib(miner_power.qa_power),

                   total_power.qa_power,
                   makeKib(total_power.qa_power),
                   bigdiv(miner_power.qa_power * 100, total_power.qa_power));
      } else {
        fmt::print(
            "{}({})\n", total_power.qa_power, makeKib(total_power.qa_power));
      }
    }
  };

}  // namespace fc::cli::cli_node
