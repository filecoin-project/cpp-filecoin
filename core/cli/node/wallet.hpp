/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/json.hpp"
#include "cli/node/node.hpp"
#include "cli/validate/address.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "common/hexutil.hpp"
#include "common/table_writer.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/big_int.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::cli::cli_node {
  using api::KeyInfo;
  using api::MarketBalance;
  using api::MsgWait;
  using common::hex_lower;
  using common::unhex;
  using crypto::signature::Signature;
  using primitives::BigInt;
  using primitives::GasAmount;
  using primitives::Nonce;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::address::decodeFromString;
  using primitives::tipset::TipsetCPtr;
  using storage::ipfs::ApiIpfsDatastore;
  using vm::actor::Actor;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::state::StateTreeImpl;

  struct Node_wallet_new : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const std::string type =
          argv.empty()
              ? "secp256k1"
              : cliArgv(argv, 0, "[bls|secp256k1 (default secp256k1)]");

      const Address address =
          cliTry(api->WalletNew(type), "Creating new wallet...");

      fmt::print("{}", address);
    }
  };

  struct Node_wallet_list {
    struct Args {
      CLI_BOOL("address-only,a", "only print addresses") address_only;
      CLI_BOOL("id,i", "Output ID addresses") id;
      CLI_BOOL("market,m", "Output market balances") market;

      CLI_OPTS() {
        Opts opts;
        address_only(opts);
        id(opts);
        market(opts);
        return opts;
      }
    };
    CLI_RUN() {
      const Node::Api api{argm};

      const std::vector<Address> addresses =
          cliTry(api->WalletList(), "Getting list of wallets...");

      if (args.address_only) {
        for (const Address &address : addresses) {
          fmt::print("{}\n", address);
        }
        return;
      }

      const Address default_address = cliTry(
          api->WalletDefaultAddress(), "Getting default address of wallet...");

      TableWriter table_writer{
          "Address",
          "ID",
          {"Balance", 'r'},
          {"Market(Avail)", 'r'},
          {"Market(Locked)", 'r'},
          {"Nonce", 'r'},
          "Default",
          {"Error", 'n'},
      };

      for (const Address &address : addresses) {
        auto row{table_writer.row()};
        row["Address"] = fmt::to_string(address);

        auto maybe_actor = api->StateGetActor(address, TipsetKey{});

        if (maybe_actor.has_error()) {
          row["Error"] = maybe_actor.error().message();
          continue;
        }
        const Actor actor = maybe_actor.value();

        row["Balance"] = fmt::to_string(actor.balance);
        row["Nonce"] = fmt::to_string(actor.nonce);

        if (address == default_address) {
          row["Default"] = "X";
        }

        if (args.id) {
          auto maybe_id = api->StateLookupID(address, TipsetKey{});
          row["ID"] =
              maybe_id.has_error() ? "n/a" : fmt::to_string(maybe_id.value());
        }

        if (args.market) {
          const auto maybe_balance =
              api->StateMarketBalance(address, TipsetKey{});
          if (maybe_balance.has_value()) {
            const MarketBalance &balance = maybe_balance.value();
            row["Market(Avail)"] =
                fmt::to_string(balance.escrow - balance.locked);
            row["Market(Locked)"] = fmt::to_string(balance.locked);
          }
        }
      }

      table_writer.write(std::cout);
    }
  };

  struct Node_wallet_balance : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address address = (argv.size() == 1)
                                  ? cliArgv<Address>(argv, 0, "address")
                                  : cliTry(api->WalletDefaultAddress(),
                                           "Getting default address...");

      const TokenAmount balance =
          cliTry(api->WalletBalance(address), "Getting balance of wallet...");

      if (balance == 0) {
        fmt::print("{} (warning: may display 0 if chain sync in progress)\n",
                   balance);
      } else {
        fmt::print("{}\n", balance);
      }
    }
  };

  struct Node_wallet_addBalance {
    struct Args {
      CLI_OPTIONAL("from,f", "Address from take balance", Address) from;
      CLI_DEFAULT("gas-limit", "Limit of gas", GasAmount, {0}) gas_limit;

      CLI_OPTS() {
        Opts opts;
        from(opts);
        gas_limit(opts);
        return opts;
      }
    };
    CLI_RUN() {
      const Node::Api api{argm};

      const Address address_from = (args.from)
                                       ? *args.from
                                       : cliTry(api->WalletDefaultAddress(),
                                                "Getting default address...");
      const Address address_to{
          cliArgv<Address>(argv, 0, "Address to add balance")};
      const TokenAmount amount{
          cliArgv<TokenAmount>(argv, 1, "Amount of add balance")};

      const SignedMessage signed_message = cliTry(api->MpoolPushMessage(
          vm::message::UnsignedMessage(
              address_to, address_from, 0, amount, 0, *args.gas_limit, 0, {}),
          boost::none));

      const MsgWait message_wait =
          cliTry(api->StateWaitMsg(signed_message.getCid(), 1, 10, false),
                 "Wait message");
    }
  };

  struct Node_wallet_default : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address default_address =
          cliTry(api->WalletDefaultAddress(), "Getting default address...");
      fmt::print("{}", default_address);
    }
  };

  struct Node_wallet_setDefault : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address address{
          cliArgv<Address>(argv, 0, "Address for set as default")};

      cliTry(api->WalletSetDefault(address), "Setting default address");
    }
  };

  struct Node_wallet_import {
    struct Args {
      CLI_DEFAULT("format,f",
                  "specify input format for key [hex-lotus|json-lotus]",
                  std::string,
                  {"hex-lotus"})
      format;
      CLI_BOOL("as-default", "import the given key as your new default key")
      as_default;
      CLI_OPTS() {
        Opts opts;
        format(opts);
        as_default(opts);
        return opts;
      }
    };
    CLI_RUN() {
      const Node::Api api{argm};

      if (*args.format != "hex-lotus" && *args.format != "json-lotus") {
        throw CliError("unrecognized or unsupported format: " + *args.format);
      }

      const std::string path =
          (argv.size() == 1) ? cliArgv(
              argv, 0, "<path> (optional, will read from stdin if omitted)")
                             : "";

      Bytes input_data;

      if (path.empty()) {
        fmt::print("Enter private key: ");
        std::string private_key;
        std::cin >> private_key;

        input_data = Bytes(private_key.begin(), private_key.end());
      } else {
        input_data = cliTry(common::readFile(boost::filesystem::path(path)),
                            "Reading file...");
      }

      api::KeyInfo key_info;

      if (*args.format == "hex-lotus") {
        while (*--input_data.end() == '\n') {
          input_data.pop_back();
        }
        input_data =
            cliTry(unhex(common::span::bytestr(input_data)), "Unhex data...");
      }

      const auto json =
          cliTry(codec::json::parse(input_data), "Parse json data...");
      key_info = cliTry(api::decode<KeyInfo>(json), "Decoding json...");

      const Address address =
          cliTry(api->WalletImport(key_info), "Importing key...");

      if (args.as_default) {
        cliTry(api->WalletSetDefault(address), "Set-default...");
      }

      fmt::print("Imported key {} successfully.\n", address);
    }
  };

  struct Node_wallet_sign : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address signing_address{
          cliArgv<Address>(argv, 0, "Signing address")};
      const std::string hex_message{cliArgv(argv, 1, "Hex message")};

      const Bytes decode_message =
          cliTry(unhex(hex_message), "Decoding hex message...");
      const Signature signature =
          cliTry(api->WalletSign(signing_address, decode_message),
                 "Signing message...");

      fmt::print("{}\n", hex_lower(signature.toBytes()));
    }
  };

  struct Node_wallet_verify : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address signing_address{
          cliArgv<Address>(argv, 0, "Signing address")};
      const std::string hex_message{cliArgv(argv, 1, "Hex message")};
      const std::string signature{cliArgv(argv, 2, "Signature")};

      const Bytes decode_message =
          cliTry(unhex(hex_message), "Decoding message...");

      const Bytes signing_bytes =
          cliTry(unhex(signature), "Decoding signature...");

      const Signature signature_from_bytes =
          cliTry(Signature::fromBytes(signing_bytes),
                 "Getting signature from bytes...");
      bool flagOk = cliTry(api->WalletVerify(
          signing_address, decode_message, signature_from_bytes));
      if (flagOk) {
        fmt::print("valid\n");
      } else {
        fmt::print("invalid\nCLI Verify called with invalid signature\n");
      }
    }
  };

  struct Node_wallet_delete : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address address{cliArgv<Address>(argv, 0, "Address for delete")};

      cliTry(api->WalletDelete(address), "Deleting address...");
    }
  };

  struct Node_wallet_market_add {
    struct Args {
      CLI_OPTIONAL("from,f",
                   "Specify address to move funds from, otherwise it will use "
                   "the default wallet address",
                   Address)
      from;
      CLI_OPTIONAL("address,a",
                   "Market address to move funds to (account or miner actor "
                   "address, defaults to --from address)",
                   Address)
      address;

      CLI_OPTS() {
        Opts opts;
        from(opts);
        address(opts);
        return opts;
      }
    };
    CLI_RUN() {
      const Node::Api api{argm};
      const TokenAmount amt{cliArgv<TokenAmount>(argv, 0, "Amount")};
      const Address address_from =
          (args.from ? (*args.from)
                     : cliTry(api->WalletDefaultAddress(),
                              "Getting default address..."));

      const Address address = (args.address) ? *args.address : address_from;

      fmt::print(
          "Submitting Add Balance message for amount {} for address {}\n",
          amt,
          address);

      const auto cid_signed_message = cliTry(
          api->MarketAddBalance(address_from, address, amt), "Add balance...");
      const MsgWait message_wait = cliTry(
          api->StateWaitMsg(cid_signed_message, 1, 10, false), "Wait message");

      fmt::print("Add balance message cid : {}\n", cid_signed_message);
    }
  };
}  // namespace fc::cli::cli_node
