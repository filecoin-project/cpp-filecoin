//
// Created by Ruslan Gilvanov  on 12.02.2022.

#pragma once
#include "cli/node/node.hpp"

namespace fc::cli::_node {
  using api::FileRef;
  using api::ImportRes;
  using api::RetrievalOrder;
  using primitives::BigInt;
  using primitives::address::Address;
  using primitives::address::decodeFromString;

  struct clientRetrieve {
    struct Args {
      std::string from;
      std::string piece_cid;
      BigInt max_funds;
      boost::filesystem::path path;
      std::string provider;
      bool car{};

      CLI_OPTS() {
        Opts opts;
        auto option{opts.add_options()};
        option("from", po::value(&from), "transaction from");
        option("piece-CID", po::value(&piece_cid), "cid of piece to retrieve");
        option(
            "maxPrice",
            po::value(&max_funds),
            "specifies the maximum token amount that  client ready to spend");
        option("path",
               po::value(&path)->required(),
               "specifies the path to retrieve");
        option("provider",
               po::value(&provider)->required(),
               "specifies retrieval provider to perform a deal");
        option("car",
               po::bool_switch(&car),
               "store result of retrieval deal to car file");
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
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

  struct clientImportData {
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


  struct clientGenerateCar{
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

  struct clientLocal: Empty{
    CLI_RUN(){
      Node::Api api {argm};
      CLI_TRY_TEXT(result, api._->ClientListImports(), "Fail of getting imports list");
      for(auto it = result.begin(); it != result.end(); it++){
        std::cout<<"Root CID: "<<it->root.toString().value()<<"\n";
        std::cout<<"Source: "<<it->source<<"\n";
        std::cout<<"Path: "<<it->path<<"\n";
      }
    }
  };



}  // namespace fc::cli::_node
