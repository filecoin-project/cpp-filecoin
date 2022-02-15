/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "cli/node/node.hpp"

namespace fc::cli::_node {
  using api::FileRef;
  using api::ImportRes;
  using api::RetrievalOrder;
  using primitives::BigInt;
  using primitives::address::Address;
  using primitives::address::decodeFromString;

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
      CLI_TRY_TEXT(root, CID::fromString(args.root), "Invalid root CID: " + args.root);
      CLI_TRY_TEXT(cid,
                   CID::fromString(args.piece_cid),
                   "Invalid piece CID: " + args.piece_cid)
      if (!args.provider.empty()) {
        CLI_TRY_TEXT(miner,
                     decodeFromString(args.provider),
                     "Invalid Provider Address: " + args.provider)
      }
      if (!args.from.empty()) {
        CLI_TRY_TEXT(client,
                     decodeFromString(args.from),
                     "Invalid Client Address: " + args.from)
      }
      // TODO: continue function
      // TODO: positional args
    }
  };

  struct Node_client_importData {
    struct Args {
      bool car{};
      std::string path;

      CLI_OPTS() {
        Opts opts;
        auto option{opts.add_options()};
        option("car",
               po::bool_switch(&car),
               "import from a car file instead of a regular file");
        option("path,p", po::value(&path), "path to import");
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
      FileRef file_ref{args.path, args.car};
      CLI_TRY_TEXT(result, api._->ClientImport(file_ref), "Fail of data import")
      std::cout << "File Root CID: " << result.root.toString().value() << "\n";
      std::cout << "Data Import Success\n";
    }
  };


  struct Node_client_generateCar{
    struct  Args{
      std::string  in_path;
      std::string  out_path;
      CLI_OPTS(){
        Opts opts;
        auto option{opts.add_options()};
        option("input-path, inp", po::value(&in_path), "specifies path to source file");
        option("output-path, outp", po::value(&in_path), "specifies path to generated file");
      }

      CLI_RUN(){
        std::cout<<"Does not supported yet\n";
      }
    };
  };

  struct Node_client_local: Empty{
    CLI_RUN(){
      Node::Api api {argm};
      auto result = cliTry(api._->ClientListImports(), "Getting imports list");
      for(auto it = result.begin(); it != result.end(); it++){
        std::cout<<"Root CID: "<<it->root.toString().value()<<"\n";
        std::cout<<"Source: "<<it->source<<"\n";
        std::cout<<"Path: "<<it->path<<"\n";
      }
    }
  };

  struct Node_client_find{
    struct Args{

      CLI_OPTS(){
        Opts opts;
        auto option{opts.add_options()};

      }
    };

  };
}  // namespace fc::cli::_node
