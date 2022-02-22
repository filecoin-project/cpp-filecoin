/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/json.hpp"
#include "cli/node/node.hpp"
#include "codec/json/json.hpp"
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
  using primitives::BigInt;
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

      fmt::print(encodeToString(address));
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
          auto maybe_actor = api._->StateGetActor(address, TipsetKey{});
          auto actor = maybe_actor.value();
          if (maybe_actor.has_error()) {
            TipsetCPtr chain_head =
                cliTry(api._->ChainHead(), "Getting chain head...");
            CID state_root = chain_head->getParentStateRoot();
            // TODO add OUTCOME_TRY(network,
            // api->StateNetworkVersion(tipset_key));
            // TODO add ipfs->actor_version = actorVersion(network);
            auto ipfs = std::make_shared<ApiIpfsDatastore>(api._);
            StateTreeImpl tree{ipfs, state_root};  // TODO true?
            auto actor_exist = tree.tryGet(address);

            if (not actor_exist.has_value()) {
              table_writer.write(
                  {{"Address", encodeToString(address)},
                   {"Error", "Error"}});  // TODO add description of error
              continue;
            }
            actor.balance = 0;
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
      Address address{cliArgv<Address>(argv, 0, "address")};
      address = (argv.size() == 1)
                            ? (address)
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

  struct walletDefault : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      Address default_address =
          cliTry(api._->WalletDefaultAddress(), "Getting default address...");
      fmt::print("{}\n", encodeToString(default_address));
    }
  };

/*
  struct walletSetDefault {
    struct Args {
      CLI_OPTIONAL("address,a", "Address for set as default", Address) address;
      CLI_OPTS() {
        Opts opts;
        address(opts);
        return opts;
      }
    };

    CLI_RUN() {
      Node::Api api{argm};
      if (args.addressString.empty()) {
                throw std::invalid_argument("must pass address to set as
    default");
      }
      CLI_TRY_TEXT(address,
                   decodeFromString(args.addressString),
                   "address is incorrect");
      auto r = api._->WalletSetDefault(address);
      if (r.has_error()) {
        throw std::invalid_argument("address can`t set a default");
      }
    }
  };
  /*
        struct walletImport {
          struct Args {
            std::string path;
            std::string format = "hex-lotus";
            bool as_default = false;
            CLI_OPTS() {
              Opts opts;
              auto option{opts.add_options()};
              option("path,p",
                     po::value(&path)->required(),
                     "[<path> (optional, will read from stdin if omitted)]");
              option("format,f",
                     po::value(&format)->required(),
                     "specify input format for key");
              option("as-default",
                     po::bool_switch(&as_default),
                     "import the given key as your new default key");
              return opts;
            }
          };

          CLI_RUN() {
            Node::Api api{argm};
            Bytes input_data;

            if (args.path.empty()) {
              std::cout << "Enter private key: ";
              std::string privateKey;
              std::cin >> privateKey;
              CLI_TRY_TEXT(bytes,
                           codec::cbor::encode(privateKey),
                           "Can`t convert private key to bytes")
              input_data = bytes;
            } else {
              input_data = common::readFile(args.path).value();  // TODO add
  check
            }

            api::KeyInfo key_info;
            if (args.format == "hex-lotus" || args.format == "json-lotus") {

              auto maybe_json = codec::json::parse(input_data);
              if (not maybe_json.has_value()) {
                throw std::invalid_argument("");
              }
              auto json{std::move(maybe_json.value())};

              CLI_TRY_TEXT(key_info, api::decode<KeyInfo>(json), "Can`t decode
        json")

            } else if (args.format == "gfs-json") {
              std::cout << "gfs-json not supported\n";
            } else {
              throw std::invalid_argument("unrecognized format: " +
  args.format);
            }

            CLI_TRY_TEXT(
                address, api._->WalletImport(key_info), "Can`t import
  wallet");

            if (args.as_default) {
              CLI_TRY_TEXT1(api._->WalletSetDefault(address),
                            "failed to set default key")
            }

            std::cout << "imported key " + encodeToString(address)
                             + "successfully!\n";
          }
        };

        struct walletSign {
          struct Args {
            std::string signAddress;
            std::string hexMessage;

            CLI_OPTS() {
              Opts opts;
              auto option{opts.add_options()};
              option("address,a",
                     po::value(&signAddress)->required(),
                     "Signing address");
              option("message,m", po::value(&hexMessage)->required(), "Hex
        message"); return opts;
            }
          };

          CLI_RUN() {
            Node::Api api{argm};
            if (args.signAddress.empty() || args.hexMessage.empty()) {
              throw std::invalid_argument(
                  "must specify signing address and message to sign");
            }

            CLI_TRY_TEXT(
                address, decodeFromString(args.signAddress), "Can`t get
  address") CLI_TRY_TEXT( decode_message, unhex(args.hexMessage), "Can`t
  decode hex message");

            CLI_TRY_TEXT(signed_message,
                         api._->WalletSign(address, decode_message),
                         "Can`t sign message")

            std::cout << hex_lower(signed_message.toBytes()) << '\n';
          }
        };

        struct walletVerify {
          struct Args {
            std::string signing_address;
            std::string hex_message;
            std::string signature;
            CLI_OPTS() {
              Opts opts;
              auto option{opts.add_options()};
              option("signing_address,a",
                     po::value(&signing_address)->required(),
                     "Signing address");
              option("message,m", po::value(&hex_message)->required(), "Hex
        message"); option("signature,s", po::value(&signature)->required(),
        "Signature"); return opts;
            }
          };

          CLI_RUN() {
            Node::Api api{argm};

            if (args.signing_address.empty() || args.hex_message.empty()
                || args.signature.empty()) {
              throw std::invalid_argument(
                  "must specify signing address, message, and signature to
       verify");
            }
            CLI_TRY_TEXT(
                address, decodeFromString(args.signing_address), "Can`t get
        address") CLI_TRY_TEXT( decode_message, unhex(args.hex_message),
  "Can`t decode hex message"); CLI_TRY_TEXT( signing_bytes,
  unhex(args.signature), "Can`t decode signature");  // TODO maybe
  signature.fromBytes?
            // TODO

            CLI_TRY_TEXT(signature,
                         Signature::fromBytes(signing_bytes),
                         "Can`t get signature from bytes");
            CLI_TRY_TEXT(flagOk,
                         api._->WalletVerify(address, decode_message,
  signature), "Can`t verify"); if (flagOk) { std::cout << "valid\n"; } else {
              std::cout << "invalid\nCLI Verify called with invalid
  signature\n";
            }
          }
        };

        struct walletDelete {
          struct Args {
            std::string address;

            CLI_OPTS() {
              Opts opts;
              auto option{opts.add_options()};
              option(
                  "address,a", po::value(&address)->required(), "Address for
        delete"); return opts;
            }
          };

          CLI_RUN() {
            Node::Api api{argm};
            if (args.address.empty()) {
              throw std::invalid_argument("must specify address to delete");
            }
            CLI_TRY_TEXT(
                address, decodeFromString(args.address), "address is
  incorrect")

            CLI_TRY_TEXT1(api._->WalletDelete(address),
                          "Can`t delete address")  // TODO write WalletDelete
          }
        };

        struct walletWithdraw {
          struct Args {
            std::string amount;
            std::string wallet;
            std::string address;
            int confidence;
            CLI_OPTS() {
              Opts opts;
              auto option{opts.add_options()};
              option("amount",
                     po::value(&amount)->required(),
                     "Amount (FIL) optional, otherwise will withdraw max
        available"); option("wallet,w", po::value(&wallet)->required(),
  "Specify address to withdraw funds to, otherwise it will use " "the default
  wallet address"); option("address,a", po::value(&address)->required(),
  "Market address to withdraw from (account or miner actor " "address,
  defaults to
        --wallet address)"); option("confidence",
                     po::value(&confidence)->required(),
                     "Number of block confirmations to wait for");
              return opts;
            }
          };

          CLI_RUN() {
            Node::Api api{argm};
            Address wallet;
            if (args.wallet.empty()) {
              CLI_TRY_TEXT(walletTemp,
                           decodeFromString(args.wallet),
                           "parsing from address: " + args.wallet + "
  invalid"); wallet = walletTemp; } else { CLI_TRY_TEXT(walletTemp,
                           api._->WalletDefaultAddress(),
                           "getting default wallet address invalid");
              wallet = walletTemp;
            }

            Address address = wallet;
            if (not args.address.empty()) {
              CLI_TRY_TEXT(addressTemp,
                           decodeFromString(args.address),
                           "parsing market address: " + args.address + "
       invalid"); address = addressTemp;
            }

            CLI_TRY_TEXT(
                balance,
                api._->StateMarketBalance(address, TipsetKey{}),
                "getting market balance for address " + args.address + "
       invalid");

            BigInt available = balance.escrow - balance.locked;
            if (available <= 0) {
              throw std::invalid_argument(
                  "no funds available to withdraw");  // TODO add description
  like
                                                      // lotus
            }

            BigInt amount = available;
            if (not args.amount.empty()) {
              amount = TokenAmount(args.amount);
            }

            if (amount <= 0) {
              throw std::invalid_argument("amount must be > 0");
            }

            if (amount > available) {
              throw std::invalid_argument(
                  "can't withdraw more funds than available; requested: "
                  + lexical_cast<std::string>(available));
            }

            std::cout << "Submitting WithdrawBalance message for amount "
                      << lexical_cast<std::string>(amount) << " for address "
                      << encodeToString(wallet) << "\n";

            CLI_TRY_TEXT(signed_message,
                         api._->MarketWithdraw(wallet, address, amount),
                         "Funds manager withdr  aw error");
            std::cout << "WithdrawBalance message cid: " << signed_message <<
       "\n";

            CLI_TRY_TEXT(wait,
                         api._->StateWaitMsg(signed_message,
                                             lexical_cast<uint64_t>(args.confidence)),
                         "Wait message error");  // TODO correct?

            if (wait.receipt.exit_code != 0) {
              throw std::invalid_argument("withdrawal failed!");
            }

            CLI_TRY_TEXT(network_version,
                         api._->StateNetworkVersion(wait.tipset),
                         "Can`t get network version");

            if (network_version >= NetworkVersion::kVersion14) {
              TokenAmount withdrawn;
              // TODO if err :=
              // withdrawn.UnmarshalCBOR(bytes.NewReader(wait.Receipt.Return))
              std::cout << "Successfully withdrew "
                        << lexical_cast<std::string>(withdrawn) << "\n";
              if (withdrawn < amount) {
                std::cout << "Note that this is less than the requested amount
  of
       "
                          << lexical_cast<std::string>(withdrawn) << "\n";
              }
            }
          }
        };

        struct walletAdd {
          struct Args {
            std::string amount;
            std::string from;
            std::string address;
            CLI_OPTS() {
              Opts opts;
              auto option{opts.add_options()};
              option("address,a",
                     po::value(&address)->required(),
                     "Market address to move funds to (account or miner actor
  " "address, defaults to --from address)"); option("from,f",
                     po::value(&from)->required(),
                     "Specify address to move funds from, otherwise it will
  use the " "default wallet address"); option("amount",
        po::value(&amount)->required(), "amount"); return opts;
            }
          };
          CLI_RUN() {
            Node::Api api{argm};
            if (args.amount.empty()) {
              throw std::invalid_argument("must pass amount to add");
            }

            TokenAmount amt(args.amount);

            Address addressFrom;
            if (args.from.empty()) {
              CLI_TRY_TEXT(tempAddress,
                           api._->WalletDefaultAddress(),
                           "Can`t find default address")
              addressFrom = tempAddress;
            } else {
              CLI_TRY_TEXT(tempAddress,
                           decodeFromString(args.from),
                           "parsing from address: " + args.from)
              addressFrom = tempAddress;
            }

            Address address = addressFrom;
            if (not args.address.empty()) {
              CLI_TRY_TEXT(tempAddress,
                           decodeFromString(args.address),
                           "parsing from address: " + args.from)
              address = tempAddress;
            }

            std::cout << "Submitting Add Balance message for amount " << amt
                      << " for address " << encodeToString(address) << "\n";

            CLI_TRY_TEXT(smsg,
                         api._->MarketAddBalance(
                             addressFrom, address, amt),  // TODO add
        MarketAddBalance "Add balance error") std::cout << "AddBalance message
       cid: " << smsg << '\n';
          }
        };
      */
}  // namespace fc::cli::_node
