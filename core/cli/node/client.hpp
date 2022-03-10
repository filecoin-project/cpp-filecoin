/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <inttypes.h>
#include "api/full_node/node_api.hpp"
#include "cli/node/node.hpp"
#include "cli/validate/address.hpp"
#include "cli/validate/cid.hpp"
#include "common/enum.hpp"
#include "markets/storage/mk_protocol.hpp"
#include "primitives/atto_fil.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "storage/ipld/memory_indexed_car.hpp"
#include "storage/unixfs/unixfs.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/states/verified_registry/verified_registry_actor_state.hpp"
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor.hpp"

namespace fc::cli::cli_node {
  using api::FileRef;
  using api::FullNodeApi;
  using api::ImportRes;
  using api::RetrievalOrder;
  using api::StartDealParams;
  using boost::lexical_cast;
  using common::toString;
  using ::fc::storage::car::makeCar;
  using ::fc::storage::unixfs::wrapFile;
  using markets::storage::DataRef;
  using markets::storage::StorageDealStatus;
  using primitives::AttoFil;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::address::Address;
  using primitives::address::encodeToString;
  using primitives::piece::UnpaddedPieceSize;
  using proofs::padPiece;
  using storage::ipfs::ApiIpfsDatastore;
  using vm::VMExitCode;
  using vm::actor::kVerifiedRegistryAddress;
  using vm::actor::builtin::states::VerifiedRegistryActorStatePtr;

  const ChainEpoch kLoopback = 100;  // TODO: lookback

  StoragePower checkNotary(std::shared_ptr<FullNodeApi> api,
                           const Address &vaddr) {
    auto vid =
        cliTry(api->StateLookupID(vaddr, TipsetKey()),
               "Getting IPLD id of data associated with provided address...");
    auto actor =
        cliTry(api->StateGetActor(kVerifiedRegistryAddress, TipsetKey()),
               "Getting VerifierActor");
    auto ipfs = std::make_shared<ApiIpfsDatastore>(api);
    auto version = cliTry(api->StateNetworkVersion(TipsetKey()),
                          "Getting Chain Version...");
    auto state =
        cliTry(getCbor<VerifiedRegistryActorStatePtr>(ipfs, actor.head));
    auto res = cliTry(cliTry(state->getVerifiedClientDataCap(vid)),
                      "Client {} isn't in notary tables",
                      vaddr);
    return res;
  }

  struct Node_client_retrieve {
    struct Args {
      CLI_OPTIONAL("from", "", Address) from;
      CLI_OPTIONAL("provider", "", Address) provider;
      CLI_OPTIONAL("pieceCid", "", std::string) piece_cid;
      CLI_OPTIONAL("maxPrice", "", AttoFil) max_price;
      CLI_OPTIONAL("data-selector", "", std::string) data_selector;
      CLI_BOOL("car", "") car;
      CLI_BOOL("allow-local", "") allow_local;
      CLI_BOOL("car-export-merkle-proof", "") car_export_merkle_proof;

      CLI_OPTS() {
        Opts opts;
        from(opts);
        provider(opts);
        piece_cid(opts);
        allow_local(opts);
        car(opts);
        max_price(opts);
        data_selector(opts);
        car_export_merkle_proof(opts);
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
      RetrievalOrder order{};
      auto data_cid{cliArgv<CID>(argv, 0, "dataCid")};
      auto path{cliArgv<boost::filesystem::path>(argv, 1, "path")};
      order.client =
          (args.from ? *args.from
                     : cliTry(api->WalletDefaultAddress(),
                              "Getting address of default wallet..."));
      order.miner = (args.provider ? *args.provider : Address{});
      if (args.max_price) {
        fmt::print("max price is {}fil ({}attofil)\n",
                   args.max_price->fil,
                   args.max_price->atto());
      }
      fmt::print("retrieving {} to {} {}\n",
                 data_cid,
                 args.car ? "car" : "file",
                 path);

      // TODO: continue function
    }
  };

  struct Node_client_importData {
    struct Args {
      CLI_BOOL("car", "import from a car file instead of a regular file") car;

      CLI_OPTS() {
        Opts opts;
        car(opts);
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
      auto path{cliArgv<std::string>(argv, 0, "path to file to import")};
      FileRef file_ref{path, args.car};
      auto result =
          cliTry(api->ClientImport(file_ref), "Processing data import");
      fmt::print("File Root CID: " + result.root.toString().value());
      fmt::print("Data Import Success");
    }
  };

  struct Node_client_deal {
    struct Args {
      CLI_OPTIONAL("manual-piece-cid",
                   "manually specify piece commitment for data (dataCid must "
                   "be to a car file)",
                   CID)
      man_piece_cid;
      CLI_DEFAULT("manual-piece-size",
                  "if manually specifying piece cid, used to specify size "
                  "(dataCid must be to a car file)",
                  uint64_t,
                  {0})
      man_piece_size;
      CLI_BOOL("manual-stateless-deal",
               "instructs the node to send an offline deal without registering "
               "it with the deallist/fsm")
      man_stateless;
      CLI_OPTIONAL("from", "specify address to fund the deal with", Address)
      from;
      CLI_DEFAULT("start-epoch",
                  "specify the epoch that the deal should start at",
                  ChainEpoch,
                  {-1})
      start_epoch;
      CLI_DEFAULT("cid-base",
                  "Multibase encoding used for version 1 CIDs in output.",
                  std::string,
                  {"base-32"})
      cid_base;
      CLI_BOOL("fast-retrieval",
               "indicates that data should be available for fast retrieval")
      fast_ret;
      CLI_BOOL("verified-deal",
               "indicate that the deal counts towards verified client tota")
      verified_deal;
      CLI_DEFAULT(
          "provider-collateral",
          "specify the requested provider collateral the miner should put up",
          TokenAmount,
          {0})
      collateral;

      CLI_OPTS() {
        Opts opts;
        man_piece_cid(opts);
        man_piece_size(opts);
        man_stateless(opts);
        from(opts);
        start_epoch(opts);
        cid_base(opts);
        fast_ret(opts);
        verified_deal(opts);
        collateral(opts);
        return opts;
      }
    };

    CLI_RUN() {
      ChainEpoch kMinDealDuration{60};  // TODO: read from config;
      ChainEpoch kMaxDealDuration{160};
      auto data_cid{cliArgv<CID>(
          argv, 0, "dataCid comes from running 'lotus client import")};
      auto miner{cliArgv<Address>(
          argv, 1, "address of the miner you wish to make a deal with")};
      auto price{
          cliArgv<TokenAmount>(argv, 2, "price calculated in FIL/Epoch")};
      auto duration{cliArgv<ChainEpoch>(
          argv, 3, "is a period of storing the data for, in blocks")};
      Node::Api api{argm};
      if (duration < kMinDealDuration)
        throw CliError("Minimal deal duration is {}", kMinDealDuration);
      if (duration < kMaxDealDuration)
        throw CliError("Max deal duration is {}", kMaxDealDuration);
      DataRef data_ref;
      Address address_from =
          args.from ? *args.from : cliTry(api->WalletDefaultAddress());
      if (args.man_piece_cid) {
        UnpaddedPieceSize piece_size{*args.man_piece_size};
        data_ref = {.transfer_type = "manual",
                    .root = data_cid,
                    .piece_cid = *args.man_piece_cid,
                    .piece_size = piece_size};
      } else {
        data_ref = {.transfer_type = "graphsync", .root = data_cid};
      }
      auto dcap =
          cliTry(api->StateVerifiedClientStatus(address_from, TipsetKey()),
                 "Failed to get status of {}",
                 address_from);
      bool isVerified = dcap.has_value();
      if (args.verified_deal && !isVerified)
        throw CliError(
            "Cannot perform verified deal using unverified address {}",
            address_from);
      StartDealParams deal_params = {.data = data_ref,
                                     .wallet = address_from,
                                     .miner = miner,
                                     .epoch_price = price,
                                     .min_blocks_duration = duration,
                                     .deal_start_epoch = *args.start_epoch,
                                     .fast_retrieval = args.fast_ret,
                                     .verified_deal = isVerified,
                                     .provider_collateral = *args.collateral};
      auto proposal_cid = cliTry(api->ClientStartDeal(deal_params));
      fmt::print("Deal proposal CID: {}\n",
                 cliTry(proposal_cid.toString(), "Cannot extract CID"));
    }
  };

  struct Node_client_generateCar : Empty {
    CLI_RUN() {
      auto path_to{cliArgv<boost::filesystem::path>(argv, 0, "input-path")};
      auto path_out{cliArgv<boost::filesystem::path>(argv, 1, "output-path")};

      auto tmp_path = path_to.string() + ".unixfs-tmp.car";
      auto ipld = cliTry(MemoryIndexedCar::make(tmp_path, true),
                         "Creting IPLD instance of {}",
                         tmp_path);
      std::ifstream file{path_to.string()};
      auto root = cliTry(wrapFile(*ipld, file));
      cliTry(::fc::storage::car::makeSelectiveCar(
          *ipld, {{root, {}}}, path_out.string()));
      boost::filesystem::remove(tmp_path);
      padPiece(path_out);
    }
  };

  struct Node_client_local : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      auto result = cliTry(api->ClientListImports(), "Getting imports list");
      for (auto it = result.begin(); it != result.end(); it++) {
        fmt::print("Root CID: {} \n", it->root.toString().value());
        fmt::print("Source: {}\n", it->source);
        fmt::print("Path: {}\n", it->path);
      }
    }
  };

  struct Node_client_find {
    struct Args {
      CLI_OPTIONAL("piece-cid",
                   "require data to be retrieved from a specific Piece CID",
                   CID)
      piece_cid;
      CLI_OPTS() {
        Opts opts;
        piece_cid(opts);
      }
    };
    CLI_RUN() {
      auto data_cid{cliArgv<CID>(argv, 0, "data-cid")};
      Node::Api api{argm};
      auto querry_offers =
          cliTry(api->ClientFindData(data_cid, *args.piece_cid));
      for (const auto &offer : querry_offers) {
        if (offer.error == "") {
          fmt::print("ERROR: {}@{}: {}\n",
                     encodeToString(offer.miner),
                     offer.peer.peer_id.toHex(),
                     offer.error);
        } else {
          fmt::print("RETRIEVAL: {}@{}-{}-{}\n",
                     encodeToString(offer.miner),
                     offer.peer.peer_id.toHex(),
                     offer.min_price,
                     offer.size);
        }
      }
    }
  };

  struct Node_client_listRetrievals {  // TODO: Done
    struct Args {
      CLI_BOOL("verbose", "print verbose deal details") verbose;
      CLI_BOOL("show-failed", "show failed/failing deals") failed_show;
      CLI_BOOL("completed", "show completed retrievals") completed_show;

      CLI_OPTS() {
        Opts opts;
        verbose(opts);
        failed_show(opts);
        completed_show(opts);
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
      bool failed_show = !args.failed_show;
      // TODO: continue;
      // chan = ()
      // if(flag){
      //   chan->read([](){
      //      print(api->stateRetrivalsDeals)
      //
      //  })
      while (true) {
        sleep(10000000000);
      }
      //
    }
  };

  struct Node_client_inspectDeal {  // TODO: continue
    struct Args {
      CLI_OPTIONAL("proposal-cid", "proposal cid of deal to be inspected", CID)
      proposal_cid;
      CLI_OPTIONAL("deal-id", "id of deal to be inspected", int) deal_id;
      CLI_OPTS() {
        Opts opts;
        proposal_cid(opts);
        deal_id(opts);
        return opts;
      }
    };
    CLI_RUN() {
      Node::Api api{argm};
    }
  };

  struct Node_client_dealStats {
    struct Args {
      CLI_DEFAULT("newer-than",
                  "list all deals stas that was made after given period",
                  ChainEpoch,
                  {0})
      newer;
      CLI_OPTS() {
        Opts opts;
        newer(opts);
        return opts;
      }
    };
    CLI_RUN() {
      Node::Api api{argm};
      auto deals = cliTry(api->ClientListDeals());
      uint64_t total_size{0};
      std::map<StorageDealStatus, uint64_t> by_state;
      for (const auto &deal : deals) {
        // TODO(@Elestrias): [FIL-615] Check Creation time and since-epoch flag
        total_size += deal.size;
        by_state[deal.state] += deal.size;
      }
      fmt::print("Total: {} deals, {}", deals.size(), total_size);
      for (const auto &[state, size] : by_state) {
        fmt::print("Deal with status {} allocates {} bytes",
                   toString(state).value(),
                   size);
      }
    }
  };

  struct Node_client_listDeals {
    struct Args {
      CLI_BOOL("show-failed", "show failed/failing deals") failed_show;
      CLI_OPTS() {
        Opts opts;
        failed_show(opts);
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
      fmt::print("Not supported yet\n");
      auto local_deals =
          cliTry(api->ClientListDeals(), "Getting local client deals...");
      // TODO(Markuus): make output;
    }
  };

  struct Node_client_balances {
    struct Args {
      CLI_OPTIONAL("client", "specifies market  client", Address) client;
      CLI_OPTS() {
        Opts opts;
        client(opts);
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
      Address addr =
          (args.client ? *args.client
                       : cliTry(api->WalletDefaultAddress(),
                                "Getting address of default wallet..."));
      auto balance = cliTry(api->StateMarketBalance(addr, TipsetKey()));

      fmt::print("  Escrowed Funds:        {}\n",
                 lexical_cast<std::string>(balance.escrow));
      fmt::print("  Locked Funds:          {}\n",
                 lexical_cast<std::string>(balance.locked));
      fmt::print("  Avaliable:             {}\n",
                 lexical_cast<std::string>(balance.escrow - balance.locked));
    }
  };

  struct Node_client_getDeal : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      auto proposal_cid{cliArgv<CID>(argv, 0, "proposal cid of deal to get")};
      // TODO: Client getDealInfo
    }
  };

  struct Node_client_stat : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      auto cid{cliArgv<CID>(argv, 0, " cid of localy stored file")};
      // auto deal_size = cliTry(api->);
      // TODO: CLientDealSize;
    }
  };

  struct Node_client_listTransfers {
    struct Args {
      CLI_BOOL("completed", "show completed data transfers") completed;
      CLI_BOOL("watch",
               "watch deal updates in real-time, rather than a one time list")
      watch;
      CLI_BOOL("show-failed", "show failed/cancelled transfers") failed_show;

      CLI_OPTS() {
        Opts opts;
        completed(opts);
        watch(opts);
        failed_show(opts);
        return opts;
      }
    };

    CLI_RUN() {
      fmt::print("Not supported yet");
      // TODO: ClientListDataTransfers
    }
  };

  struct Node_client_grantDatacap {
    struct Args {
      CLI_OPTIONAL("from",
                   "specifies the adrress of notary to send message from",
                   Address)
      from;
      CLI_OPTS() {
        Opts opts;
        from(opts);
        return opts;
      }
    };

    CLI_RUN() {
      auto target{cliArgv<Address>(argv, 0, "target address")};
      auto allowness{cliArgv<TokenAmount>(argv, 1, "amount")};
      Node::Api api{argm};
      auto dcap = checkNotary(api.api, *args.from);
      if (dcap < allowness)
        throw CliError(
            "cannot allot more allowance than notary data cap: {} < {}",
            dcap,
            allowness);
      auto encoded_params = cliTry(codec::cbor::encode(
          vm::actor::builtin::v0::verified_registry::AddVerifiedClient::Params{
              target, allowness}));
      auto signed_message = cliTry(api->MpoolPushMessage(
          {kVerifiedRegistryAddress,
           *args.from,
           {},
           0,
           0,
           0,
           vm::actor::builtin::v0::verified_registry::AddVerifiedClient::Number,
           encoded_params},
          api::kPushNoSpec));

      fmt::print("message sent, now waiting on cid: {}",
                 signed_message.getCid());
      auto mwait = cliTry(api->StateWaitMsg(
          signed_message.getCid(), kMessageConfidence, kLoopback, false));
      if (mwait.receipt.exit_code != VMExitCode::kOk)
        throw CliError("failed to add verified client");
      fmt::print("Client {} was added successfully!", target);
    }
  };

  struct Node_client_checkClientDataCap : Empty {
    CLI_RUN() {
      auto address{cliArgv<Address>(argv, 0, "address of client")};
      Node::Api api{argm};
      auto res = cliTry(api->StateVerifiedClientStatus(address, TipsetKey()),
                        "Getting Verified Client info...");
      fmt::print("Client {} info: {}", encodeToString(address), res.value());
    }
  };

  struct Node_client_listNotaries : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      auto actor =
          cliTry(api->StateGetActor(kVerifiedRegistryAddress, TipsetKey()),
                 "Getting VerifierActor");
      auto ipfs = std::make_shared<ApiIpfsDatastore>(api.api);
      auto version = cliTry(api->StateNetworkVersion(TipsetKey()),
                            "Getting Chain Version...");
      auto state =
          cliTry(getCbor<VerifiedRegistryActorStatePtr>(ipfs, actor.head));

      cliTry(state->verifiers.visit(
          [=](auto &key, auto &value) -> outcome::result<void> {
            fmt::print("{}: {}", key, value);
            return outcome::success();
          }));
    }
  };

  struct Node_client_listClients : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      auto actor =
          cliTry(api->StateGetActor(kVerifiedRegistryAddress, TipsetKey()),
                 "Getting VerifierActor");
      auto ipfs = std::make_shared<ApiIpfsDatastore>(api.api);
      auto version = cliTry(api->StateNetworkVersion(TipsetKey()),
                            "Getting Chain Version...");
      auto state =
          cliTry(getCbor<VerifiedRegistryActorStatePtr>(ipfs, actor.head));

      cliTry(state->verified_clients.visit(
          [=](auto &key, auto &value) -> outcome::result<void> {
            fmt::print("{}: {}", key, value);
            return outcome::success();
          }));
    }
  };

  struct Node_client_checkNotaryDataCap : Empty {
    CLI_RUN() {
      auto address{cliArgv<Address>(argv, 0, "address")};
      Node::Api api{argm};
      auto dcap = checkNotary(api.api, address);
      fmt::print("DataCap amount: {}", dcap);
    }
  };

}  // namespace fc::cli::cli_node
