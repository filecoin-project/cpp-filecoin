//
// Created by Ruslan Gilvanov  on 12.02.2022.

#pragma once
#include "cli/node/node.hpp"

namespace fc::cli::_node {
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

      CLI_OPTS() {
        Opts opts;
        auto option{opts.add_options()};
        option("from", po::value(&from), "transaction from");
        option("pieceCID", po::value(&piece_cid), "cid of piece to retrieve");
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
    }
  };

}  // namespace fc::cli::_node
