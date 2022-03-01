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

  struct walletNew : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      std::string type = "secp256k1";

      if (argv.size() == 1) {
        type = std::string{cliArgv<std::string>(
            argv, 0, "[bls|secp256k1 (default secp256k1)]")};
      }

      Address address =
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
    CLI_RUN() {
      Node::Api api{argm};

      std::vector<Address> addresses =
          cliTry(api._->WalletList(), "Getting list of wallets...");

      Address default_address = cliTry(api._->WalletDefaultAddress(),
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

      for (Address &address : addresses) {
        if (args.address_only) {
          fmt::print("{}\n", encodeToString(address));
        } else {
          auto maybe_actor = api._->StateGetActor(
              address, TipsetKey{});  // TODO this place input on console log of
                                      // error. It isn`t correct
          Actor actor;
          if (maybe_actor.has_error()) {
            TipsetCPtr chain_head =
                cliTry(api._->ChainHead(), "Getting chain head...");
            CID state_root = chain_head->getParentStateRoot();
            /*
            auto network_version =
            cliTry(api._->StateNetworkVersion(chain_head->key), "Getting network
            version..."); // TODO tipsetkey? auto ipfs =
            std::make_shared<ApiIpfsDatastore>(api._); ipfs->actor_version =
            actorVersion(network_version);
             */
            auto ipfs = std::make_shared<ApiIpfsDatastore>(api._);
            StateTreeImpl tree{ipfs, state_root};
            auto actor_exist = tree.tryGet(address);

            if (not actor_exist.has_value()) {
              table_writer.write(
                  {{"Address", encodeToString(address)},
                   {"Error", "Error"}});  // TODO add description of error
              continue;
            }
            actor = maybe_actor.value();

            actor.balance = 0;
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
      Node::Api api{argm};
      Address address;
      address = (argv.size() == 1)
                    ? (Address{cliArgv<Address>(argv, 0, "address")})
                    : cliTry(api._->WalletDefaultAddress(),
                             "Getting default address...");

      TokenAmount balance =
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
      Node::Api api{argm};

      Address address_from = (args.from) ? args.from._.value()
                                         : cliTry(api._->WalletDefaultAddress(),
                                                  "Getting default address...");
      Address address_to{cliArgv<Address>(argv, 0, "Address to add balance")};
      TokenAmount amount{
          cliArgv<TokenAmount>(argv, 1, "Amount of add balance")};

      auto signed_message = cliTry(api._->MpoolPushMessage(
          vm::message::UnsignedMessage(
              address_to, address_from, 0, amount, 0, *args.gas_limit, 0, {}),
          boost::none));

      auto message_wait =
          cliTry(api._->StateWaitMsg(signed_message.getCid(), 1, 10, false),
                 "Wait message");
    }
  };

  struct walletDefault : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      Address default_address =
          cliTry(api._->WalletDefaultAddress(), "Getting default address...");
      fmt::print("{}", encodeToString(default_address));
    }
  };

  struct walletSetDefault : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      Address address{cliArgv<Address>(argv, 0, "Address for set as default")};

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
      Node::Api api{argm};

      if (*args.format != "hex-lotus" && *args.format != "json-lotus") {
        throw CliError("unrecognized or unsupported format: " + *args.format);
      }

      std::string path;
      if (argv.size() == 1) {
        path = {cliArgv<std::string>(
            argv, 0, "<path> (optional, will read from stdin if omitted)")};
      }

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

      auto json = cliTry(codec::json::parse(input_data), "Parse json data...");
      key_info = cliTry(api::decode<KeyInfo>(json), "Decoding json...");

      Address address =
          cliTry(api._->WalletImport(key_info), "Importing key...");

      if (args.as_default) {
        cliTry(api._->WalletSetDefault(address), "Set-default...");
      }

      fmt::print("Imported key {} successfully.\n", encodeToString(address));
    }
  };

  struct walletSign : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      Address signing_address{cliArgv<Address>(argv, 0, "Signing address")};
      std::string hex_message{cliArgv<std::string>(argv, 0, "Hex message")};

      auto decode_message =
          cliTry(unhex(hex_message), "Decoding hex message...");
      auto signed_message =
          cliTry(api._->WalletSign(signing_address, decode_message),
                 "Signing message...");

      std::cout << hex_lower(signed_message.toBytes()) << '\n';
    }
  };

  struct walletVerify : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      Address signing_address{cliArgv<Address>(argv, 0, "Signing address")};
      std::string hex_message{cliArgv<std::string>(argv, 1, "Hex message")};
      std::string signature{cliArgv<std::string>(argv, 2, "Signature")};

      auto decode_message = cliTry(unhex(hex_message), "Decoding message...");

      auto signing_bytes = cliTry(unhex(signature), "Decoding signature...");

      auto signature_from_bytes = cliTry(Signature::fromBytes(signing_bytes),
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
      Node::Api api{argm};
      Address address{cliArgv<Address>(argv, 0, "Address for delete")};

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
      Node::Api api{argm};
      TokenAmount amt{cliArgv<TokenAmount>(argv, 0, "Amount")};
      Address address_from = (args.from ? (args.from._.value())
                                        : cliTry(api._->WalletDefaultAddress(),
                                                 "Getting default address..."));

      Address address = address_from;
      if (args.address) {
        address = args.address._.value();
      }

      fmt::print(
          "Submitting Add Balance message for amount {} for address {}\n",
          amt,
          encodeToString(address));

      auto smsg = cliTry(api._->MarketAddBalance(address_from, address, amt),
                         "Add balance...");

      fmt::print("Add balance message cid : {}\n", smsg.value());
    }
  };
}  // namespace fc::cli::_node
