/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "cli/node/node.hpp"
#include "cli/validate/address.hpp"
#include "cli/validate/cid.hpp"
#include "cli/validate/tipset.hpp"

namespace fc::cli::cli_node {
  using api::BlockMessages;
  using api::CidMessage;
  using api::MinerInfo;
  using api::MinerPower;
  using api::MsgWait;
  using api::SignedMessage;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
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

    //    fmt::print("Executed in tipset: {}\n",
    //    fmt::join(message_wait.tipset.cids(), ", ")); // TODO error
    fmt::print("Exit Code: {}\n", message_wait.receipt.exit_code);
    fmt::print("Gas Used: {}\n", message_wait.receipt.gas_used);
    //    fmt::print("Return: {}\n\n", message_wait.receipt.return_value); //
    //    TODO ERROR becase encode value? or just print bytes
    printReceiptReturn(api, message, message_wait.receipt);
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
                 epochTime(proving_deadline.current_epoch,
                           proving_deadline.period_start));
    }
  };

  TipsetCPtr loadTipset(const Node::Api &api,
                        const boost::optional<Tipset> &tipset) {
    if (not tipset) {
      return cliTry(api->ChainHead());
    }
    // parse
  }

  using tipset_template =
      CLI_OPTIONAL("tipset,t",
                   "specify tipset to call method on (pass comma separated "
                   "array of cids)",
                   Tipset);

  struct Node_developer_networkVersion : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const NetworkVersion network_version = cliTry(
          api->StateNetworkVersion(TipsetKey()), "Getting Chain Version...");

      fmt::print("Network version: {}\n", network_version);
    }
  };

  struct Node_developer_balance : Empty {
    CLI_RUN() {
      // TODO wait to client pr
      // Node_client_balance::run(argm, args, argv);
    }
  };

  struct Node_developer_sector {
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
      fmt::print("InitialPledge: {}\n",
                 sector_info.init_pledge);  // TODO types.FIL
      fmt::print("ExpectedDayReward: {}\n",
                 sector_info.expected_day_reward);  // TODO types.FIL
      fmt::print("ExpectedStoragePledge: {}\n\n",
                 sector_info.expected_storage_pledge);  // TODO types.FIL

      const api::SectorLocation sector_partition = cliTry(
          api->StateSectorPartition(miner_address, sector_number, tipset->key),
          "Getting sector partition...");

      fmt::print("Deadline: {}\n", sector_partition.deadline);
      fmt::print("Partition: {}\n", sector_partition.partition);
    }
  };

  struct Node_developer_call {
    struct Args {
      tipset_template tipset;
      CLI_DEFAULT("from", "Address from", Address, {/*TODO ???*/}) from;
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

  struct Node_developer_searchMsg : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const CID message_cid{cliArgv<CID>(argv, 0, "Message CID")};

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

  struct Node_developer_waitMsg {
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

  struct Node_developer_sectorSize {
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

      fmt::print("{} ({})\n",
                 std::to_string(miner_info.sector_size).size(),
                 miner_info.sector_size);
    }
  };

  struct Node_developer_lookup {
    struct Args {
      CLI_OPTIONAL("tipset,t",
                   "specify tipset to call method on (pass comma separated "
                   "array of cids)",
                   Tipset)
      tipset;
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

  struct Node_developer_getActor {
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
      fmt::print("Balance:\t{}\n", actor.balance);  // TODO types.FIL
      fmt::print("Nonce:\t\t{}\n", actor.nonce);
      //      fmt::print("Code:\t\t{} ({})\n", actor.code, strtype);  // TODO
      //      strtype
      fmt::print("Head:\t\t{}\n", actor.head);
    }
  };

  struct Node_developer_listActors {
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

  struct Node_developer_listMiners {
    struct Args {
      CLI_OPTS() {
        Opts opts;
        return opts;
      }
    };

    CLI_RUN() {}
  };

  struct Node_developer_getDeal : Empty {
    CLI_RUN() {
      // TODO wait to client pr
      // Node_client_getDeal::run(argm, args, argv);
    }
  };

  struct Node_developer_activeSectors {
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

  struct Node_developer_sectors {
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

  struct Node_developer_power {
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
        // TODO
        //          if !builtin.IsStorageMinerActor(ma.Code) {
        //              return xerrors.New("provided address does not correspond
        //              to a miner actor")
        //            }
      }

      const api::MinerPower power =
          cliTry(api->StateMinerPower(miner_address, tipset->key));

      const Claim total_power = power.total;
      if (argv_empty) {
        const Claim miner_power = power.miner;
        fmt::print("{}({}) / {}({}) ~= {}\n",
                   miner_power.qa_power,
                   fmt::to_string(miner_power.qa_power).size(),

                   total_power.qa_power,
                   fmt::to_string(total_power.qa_power).size(),
                   bigdiv(miner_power.qa_power * 100, total_power.qa_power));
      } else {
        fmt::print("{}({})\n",
                   total_power.qa_power,
                   fmt::to_string(total_power.qa_power).size());
      }
    }
  };

}  // namespace fc::cli::cli_node