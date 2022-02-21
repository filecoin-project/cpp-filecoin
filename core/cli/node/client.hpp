/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "cli/node/node.hpp"
#include "storage/car/car.hpp"
#include "storage/ipld/memory_indexed_car.hpp"
#include "storage/unixfs/unixfs.hpp"

namespace fc::cli::_node {
  using api::FileRef;
  using api::ImportRes;
  using api::RetrievalOrder;
  using ::fc::storage::car::makeCar;
  using ::fc::storage::unixfs::wrapFile;
  using primitives::address::Address;
  using proofs::padPiece;

  struct Node_client_retrieve {
    struct Args {
      CLI_OPTIONAL("from", "", Address) from;
      CLI_OPTIONAL("provider", "", Address) provider;
      CLI_OPTIONAL("pieceCid", "", CID) piece_cid;
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
                     : cliTry(api._->WalletDefaultAddress(),
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
          cliTry(api._->ClientImport(file_ref), "Processing data import");
      fmt::print("File Root CID: " + result.root.toString().value());
      fmt::print("Data Import Success");
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
      auto result = cliTry(api._->ClientListImports(), "Getting imports list");
      for (auto it = result.begin(); it != result.end(); it++) {
        fmt::print("Root CID: {} \n", it->root.toString().value());
        fmt::print("Source: {}\n", it->source);
        fmt::print("Path: {}\n", it->path);
      }
    }
  };

  struct Node_client_find {};

  struct Node_client_listRetrieval {
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

  struct Node_client_inspectDeal {
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
  struct Node_client_list_deals {
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
          cliTry(api._->ClientListDeals(), "Getting local client deals...");
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
                       : cliTry(api._->WalletDefaultAddress(),
                                "Getting address of default wallet..."));
      auto balance = cliTry(api._->StateMarketBalance(addr, TipsetKey()));

      fmt::print("  Escrowed Funds:        {}\n", AttoFil{balance.escrow});
      fmt::print("  Locked Funds:          {}\n", AttoFil{balance.locked});
      // TODO: Reserved and Avaliable
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
      // auto deal_size = cliTry(api._->);
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
      auto allowness{cliArgv<AttoFil>(argv, 1, "amount")};
    }
  };

}  // namespace fc::cli::_node
