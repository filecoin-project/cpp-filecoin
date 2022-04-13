/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "api/full_node/node_api.hpp"

namespace fc::cli::cli_node {
  using api::MsgWait;
  using vm::VMExitCode;
  using vm::message::SignedMessage;

  struct Node_client_grantDatacap {
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
      const TokenAmount allowness{cliArgv<TokenAmount>(argv, 1, "amount")};

      const StoragePower data_cap = checkNotary(api.api, *args.from);
      if (data_cap < allowness)
        throw CliError(
            "cannot allow more allowance than notary data cap: {} < {}",
            data_cap,
            allowness);

      auto encoded_params = cliTry(codec::cbor::encode(
          vm::actor::builtin::v0::verified_registry::AddVerifiedClient::Params{
              target, allowness}));
      SignedMessage signed_message = cliTry(api->MpoolPushMessage(
          {kVerifiedRegistryAddress,
           *args.from,
           {},
           0,
           0,
           0,
           vm::actor::builtin::v0::verified_registry::AddVerifiedClient::Number,
           encoded_params},
          api::kPushNoSpec));

      fmt::print("message sent, now waiting on cid: {}\n",
                 signed_message.getCid());
      const MsgWait message_wait2 = cliTry(api->StateWaitMsg(
          signed_message.getCid(), kMessageConfidence, kLookBack, false));
      if (message_wait2.receipt.exit_code != VMExitCode::kOk)
        throw CliError("failed to add verified client");
      fmt::print("Client {} was added successfully!\n", target);
    }
  };

  struct Node_client_listNotaries : Empty {
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
  struct Node_client_addVerifier {
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

      const TokenAmount allowness = TokenAmount("257");
      const Actor actor =
          cliTry(api->StateGetActor(kVerifiedRegistryAddress, TipsetKey{}));
      auto ipfs = std::make_shared<ApiIpfsDatastore>(api.api);
      const NetworkVersion network =
          cliTry(api->StateNetworkVersion(TipsetKey{}));
      ipfs->actor_version = actorVersion(network);

      auto state =
          cliTry(getCbor<VerifiedRegistryActorStatePtr>(ipfs, actor.head));

      SignedMessage signed_message0 = cliTry(api->MpoolPushMessage(
          vm::message::UnsignedMessage(
              kVerifiedRegistryAddress, state->root_key, 0, 0, 0, 0, 0, {}),
          boost::none));
      const MsgWait message_wait0 =
          cliTry(api->StateWaitMsg(signed_message0.getCid(), 1, 10, false),
                 "Wait message");

      auto encoded_params1 = cliTry(codec::cbor::encode(
          vm::actor::builtin::v0::verified_registry::AddVerifier::Params{
              *args.from, allowness}));
      SignedMessage signed_message1 = cliTry(api->MpoolPushMessage(
          {kVerifiedRegistryAddress,
           state->root_key,
           {},
           0,
           0,
           0,
           vm::actor::builtin::v0::verified_registry::AddVerifier::Number,
           encoded_params1},
          api::kPushNoSpec));
      const MsgWait message_wait1 =
          cliTry(api->StateWaitMsg(signed_message1.getCid(), 1, 10, false),
                 "Wait message");
    }
  };

  struct Node_client_checkClientDataCap : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const Address address{cliArgv<Address>(argv, 0, "address of client")};

      const StoragePower storage_power =
          cliTry(cliTry(api->StateVerifiedClientStatus(address, TipsetKey()),
                        "Getting Verified Client info..."));
      fmt::print(
          "Client {} info: {}\n", encodeToString(address), storage_power);
    }
  };

  struct Node_client_listClients : Empty {
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

  struct Node_client_checkNotaryDataCap : Empty {
    CLI_RUN() {
      const Node::Api api{argm};

      const Address address{cliArgv<Address>(argv, 0, "address")};

      const StoragePower dcap = checkNotary(api.api, address);
      fmt::print("DataCap amount: {}\n", dcap);
    }
  };

}  // namespace fc::cli::cli_node
