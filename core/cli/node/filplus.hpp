/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "vm/actor/builtin/methods/verified_registry.hpp"
#include "vm/actor/builtin/states/verified_registry/verified_registry_actor_state.hpp"

namespace fc::cli::cli_node {
  using api::MsgWait;
  using storage::ipfs::ApiIpfsDatastore;
  using vm::VMExitCode;
  using vm::actor::Actor;
  using vm::actor::kVerifiedRegistryAddress;
  using vm::actor::builtin::states::VerifiedRegistryActorStatePtr;
  using vm::message::SignedMessage;
  namespace verifreg = vm::actor::builtin::verifreg;

  StoragePower checkNotary(const std::shared_ptr<FullNodeApi> &api,
                           const Address &vaddr) {
    const Address vid =
        cliTry(api->StateLookupID(vaddr, TipsetKey()),
               "Getting IPLD id of data associated with provided address...");
    const Actor actor =
        cliTry(api->StateGetActor(kVerifiedRegistryAddress, TipsetKey()),
               "Getting VerifierActor");
    const auto ipfs = std::make_shared<ApiIpfsDatastore>(api);
    const NetworkVersion version = cliTry(api->StateNetworkVersion(TipsetKey()),
                                          "Getting Chain Version...");
    ipfs->actor_version = actorVersion(version);
    const VerifiedRegistryActorStatePtr state =
        cliTry(getCbor<VerifiedRegistryActorStatePtr>(ipfs, actor.head));

    return cliTry(cliTry(state->getVerifierDataCap(vid)),
                  "Client {} isn't in notary tables",
                  vaddr);
  }

  struct Node_filplus_grantDatacap {
    struct Args {
      CLI_OPTIONAL("from",
                   "specifies the address of notary to send message from",
                   Address)
      from;
      CLI_OPTS() {
        Opts opts;
        from(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const Address target{cliArgv<Address>(argv, 0, "target address")};
      const TokenAmount allowance{cliArgv<TokenAmount>(argv, 1, "amount")};

      const StoragePower data_cap = checkNotary(api.api, *args.from);
      if (data_cap < allowance)
        throw CliError(
            "cannot allow more allowance than notary data cap: {} < {}",
            data_cap,
            allowance);

      auto encoded_params = cliTry(codec::cbor::encode(
          verifreg::AddVerifiedClient::Params{target, allowance}));
      SignedMessage signed_message =
          cliTry(api->MpoolPushMessage({kVerifiedRegistryAddress,
                                        *args.from,
                                        {},
                                        0,
                                        0,
                                        0,
                                        verifreg::AddVerifiedClient::Number,
                                        encoded_params},
                                       api::kPushNoSpec));

      fmt::print("message sent, now waiting on cid: {}\n",
                 signed_message.getCid());
      const MsgWait message_wait = cliTry(api->StateWaitMsg(
          signed_message.getCid(), kMessageConfidence, kLookBack, false));
      if (message_wait.receipt.exit_code != VMExitCode::kOk)
        throw CliError("failed to add verified client");
      fmt::print("Client {} was added successfully!\n", target);
    }
  };

  struct Node_filplus_listNotaries : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const Actor actor =
          cliTry(api->StateGetActor(kVerifiedRegistryAddress, TipsetKey()),
                 "Getting VerifierActor");
      auto ipfs = std::make_shared<ApiIpfsDatastore>(api.api);
      const NetworkVersion version = cliTry(
          api->StateNetworkVersion(TipsetKey()), "Getting Chain Version...");
      ipfs->actor_version = actorVersion(version);

      const auto state =
          cliTry(getCbor<VerifiedRegistryActorStatePtr>(ipfs, actor.head));

      cliTry(state->verifiers.visit(
          [=](auto &key, auto &value) -> outcome::result<void> {
            fmt::print("{}: {}\n", key, value);
            return outcome::success();
          }));
    }
  };

  // Only for local networks, wont work in main-net
  struct Node_filplus_addVerifier {
    struct Args {
      CLI_OPTIONAL("verifier", "address for verifier", Address)
      from;
      CLI_DEFAULT("amount",
                  "token amount for verifier (default: 257)",
                  TokenAmount,
                  {"257"})
      amount;
      CLI_OPTS() {
        Opts opts;
        from(opts);
        return opts;
      }
    };

    CLI_RUN() {
      const Node::Api api{argm};

      const Actor actor =
          cliTry(api->StateGetActor(kVerifiedRegistryAddress, TipsetKey{}));
      auto ipfs = std::make_shared<ApiIpfsDatastore>(api.api);
      const NetworkVersion network =
          cliTry(api->StateNetworkVersion(TipsetKey{}));
      ipfs->actor_version = actorVersion(network);

      auto state =
          cliTry(getCbor<VerifiedRegistryActorStatePtr>(ipfs, actor.head));

      auto encoded_params = cliTry(codec::cbor::encode(
          verifreg::AddVerifier::Params{*args.from, *args.amount}));
      const SignedMessage signed_message =
          cliTry(api->MpoolPushMessage({kVerifiedRegistryAddress,
                                        state->root_key,
                                        {},
                                        0,
                                        0,
                                        0,
                                        verifreg::AddVerifier::Number,
                                        encoded_params},
                                       api::kPushNoSpec));
      const MsgWait message_wait =
          cliTry(api->StateWaitMsg(signed_message1.getCid(), 1, 10, false),
                 "Wait message");
    }
  };

  struct Node_filplus_checkClientDataCap : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const Address address{cliArgv<Address>(argv, 0, "address of client")};

      const std::string_view process_desc = "Getting Verified Client info...";
      const StoragePower storage_power =
          cliTry(cliTry(api->StateVerifiedClientStatus(address, TipsetKey()),
                        process_desc),
                 process_desc);
      fmt::print(
          "Client {} info: {}\n", encodeToString(address), storage_power);
    }
  };

  struct Node_filplus_listClients : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const Actor actor =
          cliTry(api->StateGetActor(kVerifiedRegistryAddress, TipsetKey()),
                 "Getting VerifierActor");
      const auto ipfs = std::make_shared<ApiIpfsDatastore>(api.api);
      const NetworkVersion version = cliTry(
          api->StateNetworkVersion(TipsetKey()), "Getting Chain Version...");
      ipfs->actor_version = actorVersion(version);
      const auto state =
          cliTry(getCbor<VerifiedRegistryActorStatePtr>(ipfs, actor.head));

      cliTry(state->verified_clients.visit(
          [=](auto &key, auto &value) -> outcome::result<void> {
            fmt::print("{}: {}\n", key, value);
            return outcome::success();
          }));
    }
  };

  struct Node_filplus_checkNotaryDataCap : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const Address address{cliArgv<Address>(argv, 0, "address")};

      const StoragePower dcap = checkNotary(api.api, address);
      fmt::print("DataCap amount: {}\n", dcap);
    }
  };

}  // namespace fc::cli::cli_node
