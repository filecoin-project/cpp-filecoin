/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cinttypes>
#include "api/full_node/node_api.hpp"
#include "cli/node/node.hpp"
#include "cli/validate/address.hpp"
#include "cli/validate/cid.hpp"
#include "codec/json/json.hpp"
#include "common/cli_deal_stat.hpp"
#include "common/enum.hpp"
#include "common/table_writer.hpp"
#include "markets/storage/mk_protocol.hpp"
#include "primitives/atto_fil.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "storage/ipld/memory_indexed_car.hpp"
#include "storage/unixfs/unixfs.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::cli::cli_node {
  using api::FileRef;
  using api::FullNodeApi;
  using api::ImportRes;
  using api::MarketBalance;
  using api::QueryOffer;
  using api::RetrievalOrder;
  using api::StartDealParams;
  using api::StorageMarketDealInfo;
  using common::toString;
  using ::fc::storage::unixfs::wrapFile;
  using markets::retrieval::client::RetrievalDeal;
  using markets::storage::DataRef;
  using markets::storage::StorageDeal;
  using markets::storage::StorageDealStatus;
  using markets::storage::client::import_manager::Import;
  using primitives::AttoFil;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::StoragePower;
  using primitives::address::Address;
  using primitives::piece::UnpaddedPieceSize;
  using proofs::padPiece;

  static const ChainEpoch kLookBack = 100;
  static const auto dealDurationBounds =
      vm::actor::builtin::types::market::dealDurationBounds(
          primitives::piece::PaddedPieceSize());
  static const AttoFil kDefaultMaxRetrievePrice{.fil = 0};

  struct Node_client_retrieve {
    struct Args {
      CLI_OPTIONAL("from", "address to send transactions from", Address) from;
      CLI_OPTIONAL("provider",
                   "provider to use for retrieval, if not present it'll use "
                   "local discovery",
                   Address)
      provider;
      CLI_OPTIONAL("pieceCid",
                   "require data to be retrieved from a specific Piece CID",
                   CID)
      piece_cid;
      CLI_DEFAULT(
          "maxPrice",
          fmt::format(
              "maximum price the client is willing to consider (default: {}"
              " FIL)",
              kDefaultMaxRetrievePrice.atto())
              .c_str(),
          AttoFil,
          {kDefaultMaxRetrievePrice})
      max_price;
      CLI_OPTIONAL("data-selector",
                   "(NOT SUPPORTED) IPLD datamodel text-path selector, or IPLD "
                   "json selector",
                   std::string)
      data_selector;
      CLI_BOOL("car", "Export to a car file instead of a regular file") car;
      CLI_BOOL("allow-local", "allow use local imports") allow_local;
      CLI_BOOL("car-export-merkle-proof",
               "(requires --data-selector and --car) Export data-selector "
               "merkle proof")
      car_export_merkle_proof;

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
      const Node::Api api{argm};

      const CID data_cid{cliArgv<CID>(argv, 0, "dataCid")};
      const std::string path{cliArgv<std::string>(argv, 1, "path")};

      const boost::optional<CID> piece_cid = args.piece_cid.v;

      fmt::print("max price is {} fil ({} attofil)\n",
                 args.max_price->fil,
                 args.max_price->atto());

      fmt::print("retrieving {} to {} {}\n",
                 data_cid,
                 args.car ? "car" : "file",
                 path);

      const FileRef file_ref{.path = path, .is_car = args.car};

      const bool local_found = [&]() {
        if (args.allow_local) {
          std::vector<Import> imports = cliTry(api->ClientListImports());
          for (const Import &import : imports) {
            if (import.root == data_cid) {
              fmt::print("Root: {}\n", fmt::to_string(import.root));
              fmt::print("Local path: {}\n", import.path);
              return true;
            }
          }
        }
        return false;
      }();

      QueryOffer fin_offer;
      if (not local_found) {  // no local -> make retrieval
        if (not args.provider) {
          std::vector<QueryOffer> offers =
              cliTry(api->ClientFindData(data_cid, piece_cid));

          if (offers.empty()) {
            throw CliError("ClientFindData: data not found.");
          }

          fin_offer = *std::min_element(
              offers.begin(),
              offers.end(),
              [](const QueryOffer &first, const QueryOffer &second) {
                if (not first.error.empty()) {
                  return false;
                }
                if (not second.error.empty()) {
                  return true;
                }
                return first.min_price < second.min_price;
              });

          if (not fin_offer.error.empty()) {
            throw CliError("Error has appeared during offer evaluation");
          }
        } else {
          fin_offer = cliTry(
              api->ClientMinerQueryOffer(*args.provider, data_cid, piece_cid),
              "Cannot get retrieval offer from {}\n",
              fmt::to_string(*args.provider));
        }

        if (fin_offer.min_price > args.max_price->fil) {
          throw CliError("Cannot find suitable offer for provided proposal\n");
        }

        const RetrievalOrder order{
            .root = fin_offer.root,
            .piece = fin_offer.piece,
            .size = fin_offer.size,
            .total = fin_offer.min_price,
            .unseal_price = fin_offer.unseal_price,
            .payment_interval = fin_offer.payment_interval,
            .payment_interval_increase = fin_offer.payment_interval_increase,
            .client =
                (args.from ? *args.from
                           : cliTry(api->WalletDefaultAddress(),
                                    "Getting address of default wallet...")),
            .miner = fin_offer.miner,

            .peer = fin_offer.peer,
        };

        cliTry(api->ClientRetrieve(order, file_ref), "Retrieving...");
      }
      fmt::print("Success.\n");
    }
  };

  struct Node_client_import {
    struct Args {
      CLI_BOOL("car", "import from a car file instead of a regular file") car;

      CLI_OPTS() {
        Opts opts;
        car(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const std::string path{
          cliArgv<std::string>(argv, 0, "path to file to import")};

      const FileRef file_ref{path, args.car};
      const ImportRes imports =
          cliTry(api->ClientImport(file_ref), "Processing data import");
      fmt::print("File Root CID: {}\n", imports.root.toString().value());
      fmt::print("Data Import Success\n");
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
               "(NOT SUPPORTED) instructs the node to send an offline deal "
               "without registering "
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
                  "(NOT SUPPORTED) Multibase encoding used for version 1 CIDs "
                  "in output.",
                  std::string,
                  {"base-32"})
      cid_base;
      CLI_BOOL("fast-retrieval",
               "indicates that data should be available for fast retrieval")
      fast_ret;
      CLI_BOOL("verified-deal",
               "indicate that the deal counts towards verified client total")
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
      const Node::Api api{argm};

      const CID data_cid{cliArgv<CID>(
          argv, 0, "dataCid comes from running 'fuhon-node-cli client import")};
      const Address miner{cliArgv<Address>(
          argv, 1, "address of the miner you wish to make a deal with")};
      const TokenAmount price{
          cliArgv<TokenAmount>(argv, 2, "price calculated in FIL/Epoch")};
      const ChainEpoch duration{cliArgv<ChainEpoch>(
          argv, 3, "is a period of storing the data for, in blocks")};

      if (duration < dealDurationBounds.min)
        throw CliError("Minimal deal duration is {}", dealDurationBounds.min);
      if (duration > dealDurationBounds.max)
        throw CliError("Max deal duration is {}", dealDurationBounds.max);

      DataRef data_ref;
      const Address address_from =
          args.from ? *args.from : cliTry(api->WalletDefaultAddress());

      if (args.man_piece_cid) {
        data_ref = {.transfer_type = "manual",
                    .root = data_cid,
                    .piece_cid = *args.man_piece_cid,
                    .piece_size = UnpaddedPieceSize{*args.man_piece_size}};
      } else {
        data_ref = {.transfer_type = "graphsync", .root = data_cid};
      }

      const boost::optional<StoragePower> dcap =
          cliTry(api->StateVerifiedClientStatus(address_from, TipsetKey()),
                 "Failed to get status of {}",
                 address_from);
      const bool isVerified = dcap.has_value();
      if (args.verified_deal && !isVerified)
        throw CliError(
            "Cannot perform verified deal using unverified address {}",
            address_from);

      const StartDealParams deal_params = {
          .data = data_ref,
          .wallet = address_from,
          .miner = miner,
          .epoch_price = price,
          .min_blocks_duration = duration,
          .provider_collateral = *args.collateral,
          .deal_start_epoch = *args.start_epoch,
          .fast_retrieval = args.fast_ret,
          .verified_deal = isVerified};
      const CID proposal_cid = cliTry(api->ClientStartDeal(deal_params));
      fmt::print("Deal proposal CID: {}\n",
                 cliTry(proposal_cid.toString(), "Cannot extract CID"));
    }
  };

  struct Node_client_generateCar : Empty {
    using Path = boost::filesystem::path;
    CLI_RUN() {
      const Path path_in{cliArgv<Path>(argv, 0, "input-path")};
      const Path path_out{cliArgv<Path>(argv, 1, "output-path")};

      if (not boost::filesystem::exists(path_in)) {
        throw CliError("Input file does not exist.\n");
      }
      if (not boost::filesystem::is_regular_file(path_in)) {
        throw CliError("Provided path does not leads to regular file.\n");
      }

      const std::string tmp_path = path_in.string() + ".unixfs-tmp.car";
      auto ipld = cliTry(MemoryIndexedCar::make(tmp_path, true),
                         "Creting IPLD instance of {}",
                         tmp_path);

      std::ifstream file{path_in.string()};
      const CID root = cliTry(wrapFile(*ipld, file));
      cliTry(::fc::storage::car::makeSelectiveCar(
          *ipld, {{root, {}}}, path_out.string()));
      boost::filesystem::remove(tmp_path);
      padPiece(path_out);
    }
  };

  struct Node_client_local : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const std::vector<Import> clients =
          cliTry(api->ClientListImports(), "Getting imports list");
      for (const Import &client : clients) {
        fmt::print("Root CID: {} \n", client.root.toString().value());
        fmt::print("Source: {}\n", client.source);
        fmt::print("Path: {}\n\n", client.path);
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
        return opts;
      }
    };
    CLI_RUN() {
      const Node::Api api{argm};

      const CID data_cid{cliArgv<CID>(argv, 0, "data-cid")};

      const std::vector<QueryOffer> query_offers =
          cliTry(api->ClientFindData(data_cid, args.piece_cid.v));
      for (const QueryOffer &offer : query_offers) {
        if (not offer.error.empty()) {
          fmt::print("ERROR: {}@{}: {}\n",
                     offer.miner,
                     offer.peer.peer_id.toHex(),
                     offer.error);
        } else {
          fmt::print("RETRIEVAL: {}@{}-{}-{}\n",
                     offer.miner,
                     offer.peer.peer_id.toHex(),
                     offer.min_price,
                     offer.size);
        }
      }
    }
  };

  struct Node_client_listRetrievals {
    struct Args {
      CLI_BOOL("verbose", "print verbose deal details") verbose;
      CLI_BOOL("show-failed", "(NOT SUPPORTED) show failed/failing deals")
      failed_show;
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
      const Node::Api api{argm};

      const std::vector<RetrievalDeal> deals =
          cliTry(api->ClientListRetrievals(),
                 "Cannot get retrieval deals from market");
      TableWriter table_writer{
          "PayloadCID",
          "DealId",
          {"Provider", 'r'},
          {"Status", 'r'},
          {"PricePerByte", 'r'},
          {"Received", 'r'},
          "TotalPaid",
      };

      for (const RetrievalDeal &retrieval : deals) {
        const std::string cid =
            cliTry(retrieval.proposal.payload_cid.toString());
        auto row{table_writer.row()};
        row["PayloadCID"] = cid;
        row["DealID"] = fmt::to_string(retrieval.proposal.deal_id);
        row["Provider"] = fmt::to_string(retrieval.miner_wallet);
        row["Status"] = retrieval.accepted ? "accepted" : "not accepted";
        row["PricePerByte"] =
            fmt::to_string(retrieval.proposal.params.price_per_byte);
        row["Received"] =
            retrieval.all_blocks ? "all blocks" : "not all blocks";
      }
      table_writer.write(std::cout);
    }
  };

  struct Node_client_inspectDeal {
    struct Args {
      CLI_OPTIONAL("proposal-cid", "proposal cid of deal to be inspected", CID)
      proposal_cid;
      CLI_OPTIONAL("deal-id", "id of deal to be inspected", DealId) deal_id;
      CLI_OPTS() {
        Opts opts;
        proposal_cid(opts);
        deal_id(opts);
        return opts;
      }
    };
    CLI_RUN() {
      const Node::Api api{argm};

      const std::vector<StorageMarketDealInfo> deals =
          cliTry(api->ClientListDeals());
      StorageMarketDealInfo result;
      if (args.deal_id) {
        for (const auto &deal : deals) {
          if (deal.deal_id == *args.deal_id) {
            result = deal;
            break;
          }
        }
      } else {
        result = cliTry(api->ClientGetDealInfo(*args.proposal_cid),
                        "Cannot get deal info for {}",
                        fmt::to_string(*args.proposal_cid));
      }

      TableWriter table_writer{
          "Deal ID",
          "Proposal CID",
          {"Status", 'r'},
          {"Expected Duration", 'r'},
          {"Verified", 'r'},

      };
      auto row{table_writer.row()};
      row["Deal ID"] = fmt::to_string(result.deal_id);
      row["Proposal CID"] = fmt::to_string(result.proposal_cid);
      row["Status"] = fmt::to_string(result.state);
      row["Expected Duration"] = fmt::to_string(result.duration);
      row["Verified"] = fmt::to_string(result.verified);
      table_writer.write(std::cout);
    }
  };

  struct Node_client_dealStats {
    struct Args {
      CLI_DEFAULT("newer-than",
                  "list all deals stats that was made after given period",
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
      const Node::Api api{argm};

      const std::vector<StorageMarketDealInfo> deals =
          cliTry(api->ClientListDeals());
      uint64_t total_size{0};
      std::map<StorageDealStatus, uint64_t> by_state;
      for (const StorageMarketDealInfo &deal : deals) {
        // TODO(@Elestrias): [FIL-615] Check Creation time and since-epoch flag
        total_size += deal.size;
        by_state[deal.state] += deal.size;
      }
      fmt::print("Total: {} deals, {} bytes\n", deals.size(), total_size);
      for (const auto &[state, size] : by_state) {
        fmt::print("Deal with status {} allocates {} bytes\n",
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
      const Node::Api api{argm};
      const std::vector<StorageMarketDealInfo> local_deals =
          cliTry(api->ClientListDeals(), "Getting local client deals...");
      TableWriter table_writer{
          "Created",
          "Deal CID",
          "Deal ID",
          "Provider",
          "State",
          "On Chain",
          "Slashed",
          "Piece CID",
          "Size",
          "Price",
          "Duration",
          "Verified",
          "Message",
      };

      for (const StorageMarketDealInfo &deal : local_deals) {
        const StorageDeal deal_from_info =
            cliTry(api->StateMarketStorageDeal(deal.deal_id, TipsetKey()));
        auto row{table_writer.row()};
        row["Created"] = fmt::to_string(deal.creation_time);
        row["Deal CID"] = fmt::to_string(deal.proposal_cid);
        row["Deal ID"] = fmt::to_string(deal.deal_id);
        row["Provider"] = fmt::to_string(deal.provider);
        row["State"] = fmt::to_string(deal.state);
        row["On Chain"] =
            deal_from_info.state.sector_start_epoch != -1 ? fmt::format(
                "Yes (epoch: {})", deal_from_info.state.sector_start_epoch)
                                                          : "None";
        row["Slashed"] = deal_from_info.state.slash_epoch != -1
                             ? fmt::format("Yes (epoch: {})",
                                           deal_from_info.state.slash_epoch)
                             : "None";
        row["Piece CID"] = fmt::to_string(deal.piece_cid);
        row["Size"] = fmt::to_string(deal.size);
        row["Price"] = fmt::to_string(deal.price_per_epoch);
        row["Duration"] = fmt::to_string(deal.duration);
        row["Verified"] = fmt::to_string(deal.verified);
        row["Message"] = fmt::to_string(deal.message);
      }
      table_writer.write(std::cout);
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
      const Node::Api api{argm};

      const Address address =
          (args.client ? *args.client
                       : cliTry(api->WalletDefaultAddress(),
                                "Getting address of default wallet..."));
      const MarketBalance balance =
          cliTry(api->StateMarketBalance(address, TipsetKey()));

      fmt::print("  Escrowed Funds:\t\t{}\n", balance.escrow);
      fmt::print("  Locked Funds:\t\t\t{}\n", balance.locked);
      fmt::print("  Available:\t\t\t{}\n", balance.escrow - balance.locked);
    }
  };

  struct Node_client_getDeal : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const CID proposal_cid{
          cliArgv<CID>(argv, 0, "proposal cid of deal to get")};

      const StorageMarketDealInfo deal_info =
          cliTry(api->ClientGetDealInfo(proposal_cid),
                 "No such proposal cid {}\n",
                 fmt::to_string(proposal_cid));
      auto res =
          cliTry(api->StateMarketStorageDeal(deal_info.deal_id, TipsetKey()),
                 "Can't find information about deal: {}",
                 fmt::to_string(deal_info.deal_id));

      auto jsoned =
          codec::json::encode(CliDealStat{.deal_info = deal_info, .deal = res});
      const auto bytes_json = cliTry(codec::json::format(&jsoned));
      const auto response = common::span::bytestr(bytes_json);

      fmt::print("{}\n", response);
    }
  };

  // TODO(@Elestrias): [FIL-659]clientListTransfers

}  // namespace fc::cli::cli_node
