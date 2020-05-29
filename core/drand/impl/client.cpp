/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "client.hpp"

#include "network/grpc_channel_builder.hpp"
#include "parser.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::drand, DrandSyncClientImpl::Error, e) {
  using E = fc::drand::DrandSyncClientImpl::Error;
  switch (e) {
    case E::GRPC_FAILURE:
      return "gRPC error occurred";
  }
}

namespace fc::drand {
  DrandSyncClientImpl::DrandSyncClientImpl(
      std::string host,
      size_t port,
      boost::optional<std::string> pem_root_certs)
      : host_{std::move(host)},
        port_{port},
        stub_{fc::network::createSecureClient<::drand::Public>(
            host_ + ":" + std::to_string(port_),
            (pem_root_certs ? pem_root_certs.get() : ""))} {}

  outcome::result<PublicRandResponse> DrandSyncClientImpl::publicRand(
      uint64_t round) {
    ::drand::PublicRandRequest request;
    request.set_round(round);
    grpc::ClientContext context;
    ::drand::PublicRandResponse response;
    auto status = stub_->PublicRand(&context, request, &response);
    if (not status.ok()) {
      return Error::GRPC_FAILURE;
    }
    return ProtoParser::protoToHandy(response);
  }

  outcome::result<std::vector<PublicRandResponse>>
  DrandSyncClientImpl::publicRandStream(uint64_t round) {
    ::drand::PublicRandRequest request;
    request.set_round(round);
    grpc::ClientContext context;
    auto reader = stub_->PublicRandStream(&context, request);
    std::vector<PublicRandResponse> result;
    ::drand::PublicRandResponse response;
    while (reader->Read(&response)) {
      result.push_back(ProtoParser::protoToHandy(response));
    }
    auto status = reader->Finish();
    if (not status.ok()) {
      return Error::GRPC_FAILURE;
    }
    return result;
  }

  outcome::result<GroupPacket> DrandSyncClientImpl::group() {
    ::drand::GroupRequest request;
    grpc::ClientContext context;
    ::drand::GroupPacket response;
    auto status = stub_->Group(&context, request, &response);
    if (not status.ok()) {
      return Error::GRPC_FAILURE;
    }
    return ProtoParser::protoToHandy(response);
  }
}  // namespace fc::drand
