/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/json.hpp"
#include "cli/node/node.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "common/hexutil.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/big_int.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::cli::_node {
  using api::KeyInfo;
  using boost::lexical_cast;
  using common::hex_lower;
  using common::unhex;
  using crypto::signature::Signature;
  using fc::vm::actor::Actor;
  using primitives::BigInt;
  using primitives::GasAmount;
  using primitives::Nonce;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::address::decodeFromString;
  using primitives::address::encodeToString;
  using primitives::tipset::TipsetCPtr;
  using storage::ipfs::ApiIpfsDatastore;
  using vm::message::UnsignedMessage;
  using vm::state::StateTreeImpl;
  using vm::message::SignedMessage;
  using api::MsgWait;

  struct walletNew : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      std::string type = "secp256k1";

      if (argv.size() == 1) {
        type = std::string{cliArgv<std::string>(
            argv, 0, "[bls|secp256k1 (default secp256k1)]")};
      }

      const Address address =
          cliTry(api._->WalletNew(type), "Creating new wallet...");

      fmt::print("{}", encodeToString(address));
    }
  };

  struct walletList {
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
    CLI_RUN() { // TODO express
      const Node::Api api{argm};

      const std::vector<Address> addresses =
          cliTry(api._->WalletList(), "Getting list of wallets...");

      const Address default_address = cliTry(api._->WalletDefaultAddress(),
                                       "Getting default address of wallet...");

      TableWriter table_writer =
          TableWriter({TableWriter::newColumn("Address"),
                       TableWriter::newColumn("ID"),
                       TableWriter::newColumn("Balance"),
                       TableWriter::newColumn("Market(Avail)"),
                       TableWriter::newColumn("Market(Locked)"),
                       TableWriter::newColumn("Nonce"),
                       TableWriter::newColumn("Default"),
                       TableWriter::newColumn("Error")});

      for (const Address &address : addresses) {
        if (args.address_only) {
          fmt::print("{}\n", encodeToString(address));
        } else {
          auto maybe_actor = api._->StateGetActor(
              address, TipsetKey{});

          Actor actor;
          if (maybe_actor.has_error()) {
              table_writer.write(
                  {{"Address", encodeToString(address)},
                   {"Error", "Error get actor"}});
              continue;

          } else {
            actor = maybe_actor.value();
          }

          std::map<std::string, std::string> row{
              {"Address", encodeToString(address)},
              {"Balance", lexical_cast<std::string>(actor.balance)},
              {"Nonce", std::to_string(actor.nonce)}};

          if (address == default_address) {
            row["Default"] = "X";
          }

          if (args.id) {
            auto maybeId = api._->StateLookupID(address, TipsetKey{});
            if (maybeId.has_error()) {
              row["ID"] = "n/a";
            } else {
              row["ID"] = encodeToString(maybeId.value());
            }
          }

          if (args.market) {
            auto maybeBalance = api._->StateMarketBalance(address, TipsetKey{});
            if (not maybeBalance.has_error()) {
              auto balance = maybeBalance.value();
              row["Market(Avail)"] =
                  lexical_cast<std::string>(balance.escrow - balance.locked);
              row["Market(Locked)"] = lexical_cast<std::string>(balance.locked);
            }
          }

          table_writer.write(row);
        }
      }

      if (not args.address_only) {
        table_writer.flush();
      }
    }
  };

  struct walletBalance : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address address = (argv.size() == 1)
                    ? (Address{cliArgv<Address>(argv, 0, "address")})
                    : cliTry(api._->WalletDefaultAddress(),
                             "Getting default address...");

      const TokenAmount balance =
          cliTry(api._->WalletBalance(address), "Getting balance of wallet...");

      if (balance == 0) {
        fmt::print("{} (warning: may display 0 if chain sync in progress)\n",
                   balance);
      } else {
        fmt::print("{}\n", balance);
      }
    }
  };

  struct wallletAddBalance {
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

      const Address address_from = (args.from) ? *args.from
                                         : cliTry(api._->WalletDefaultAddress(),
                                                  "Getting default address...");
      const Address address_to{cliArgv<Address>(argv, 0, "Address to add balance")};
      const TokenAmount amount{
          cliArgv<TokenAmount>(argv, 1, "Amount of add balance")};

      const SignedMessage signed_message = cliTry(api._->MpoolPushMessage(
          vm::message::UnsignedMessage(
              address_to, address_from, 0, amount, 0, *args.gas_limit, 0, {}),
          boost::none));

      const MsgWait message_wait =
          cliTry(api._->StateWaitMsg(signed_message.getCid(), 1, 10, false),
                 "Wait message");
    }
  };

  struct walletDefault : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address default_address =
          cliTry(api._->WalletDefaultAddress(), "Getting default address...");
      fmt::print("{}", encodeToString(default_address));
    }
  };

  struct walletSetDefault : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address address{cliArgv<Address>(argv, 0, "Address for set as default")};

      cliTry(api._->WalletSetDefault(address), "Setting default address");
    }
  };

  struct walletImport {
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

      const std::string path = (argv.size() == 1) ?
        cliArgv<std::string>(
            argv, 0, "<path> (optional, will read from stdin if omitted)") : "";


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
            cliTry(unhex(std::string(input_data.begin(), input_data.end())),
                   "Unhex data...");
      }

      const auto json = cliTry(codec::json::parse(input_data), "Parse json data...");
      key_info = cliTry(api::decode<KeyInfo>(json), "Decoding json...");

      const Address address =
          cliTry(api._->WalletImport(key_info), "Importing key...");

      if (args.as_default) {
        cliTry(api._->WalletSetDefault(address), "Set-default...");
      }

      fmt::print("Imported key {} successfully.\n", encodeToString(address));
    }
  };

  struct walletSign : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address signing_address{cliArgv<Address>(argv, 0, "Signing address")};
      const std::string hex_message{cliArgv<std::string>(argv, 1, "Hex message")};

      const Bytes decode_message =
          cliTry(unhex(hex_message), "Decoding hex message...");
      const Signature signature =
          cliTry(api._->WalletSign(signing_address, decode_message),
                 "Signing message...");

      fmt::print("{}\n", hex_lower(signature.toBytes()));
    }
  };

  struct walletVerify : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address signing_address{cliArgv<Address>(argv, 0, "Signing address")};
      const std::string hex_message{cliArgv<std::string>(argv, 1, "Hex message")};
      const std::string signature{cliArgv<std::string>(argv, 2, "Signature")};

      const Bytes decode_message = cliTry(unhex(hex_message), "Decoding message...");

      const Bytes signing_bytes = cliTry(unhex(signature), "Decoding signature...");

      const Signature signature_from_bytes = cliTry(Signature::fromBytes(signing_bytes),
                                         "Getting signature from bytes...");
      bool flagOk = cliTry(api._->WalletVerify(
          signing_address, decode_message, signature_from_bytes));
      if (flagOk) {
        fmt::print("valid\n");
      } else {
        fmt::print("invalid\nCLI Verify called with invalid signature\n");
      }
    }
  };

  struct walletDelete : Empty {
    CLI_RUN() {
      const Node::Api api{argm};
      const Address address{cliArgv<Address>(argv, 0, "Address for delete")};

      cliTry(api._->WalletDelete(address), "Deleting address...");
    }
  };

  struct walletMarketAdd {
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
      const Address address_from = (args.from ? (*args.from)
                                        : cliTry(api._->WalletDefaultAddress(),
                                                 "Getting default address..."));

      const Address address = (args.address) ? *args.address : address_from;

      fmt::print(
          "Submitting Add Balance message for amount {} for address {}\n",
          amt,
          encodeToString(address));

      const auto cid_signed_message = cliTry(api._->MarketAddBalance(address_from, address, amt),
                         "Add balance...");
      const MsgWait message_wait =
          cliTry(api._->StateWaitMsg(cid_signed_message.value(), 1, 10, false),
                 "Wait message");

      fmt::print("Add balance message cid : {}\n", cid_signed_message.value());
    }
  };
}  // namespace fc::cli::_node
