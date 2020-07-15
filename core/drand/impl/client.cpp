/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "client.hpp"

#include <utility>

#include "network/grpc_channel_builder.hpp"
#include "parser.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::drand, DrandSyncClientImpl::Error, e) {
  using E = fc::drand::DrandSyncClientImpl::Error;
  switch (e) {
    case E::kGRPCFailure:
      return "gRPC error occurred";
  }
}

namespace fc::drand {
  DrandSyncClientImpl::DrandSyncClientImpl(
      std::string address, boost::optional<std::string> pem_root_certs)
      : address_{std::move(address)},
        stub_{fc::network::createSecureClient<::drand::Public>(
            address_, (pem_root_certs ? pem_root_certs.get() : ""))} {}

  DrandSyncClientImpl::DrandSyncClientImpl(
      const std::string &host,
      size_t port,
      boost::optional<std::string> pem_root_certs)
      : DrandSyncClientImpl{host + ":" + std::to_string(port),
                            std::move(pem_root_certs)} {}

  outcome::result<PublicRandResponse> DrandSyncClientImpl::publicRand(
      uint64_t round) {
    ::drand::PublicRandRequest request;
    request.set_round(round);
    grpc::ClientContext context;
    ::drand::PublicRandResponse response;
    auto status = stub_->PublicRand(&context, request, &response);
    if (not status.ok()) {
      return Error::kGRPCFailure;
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
      return Error::kGRPCFailure;
    }
    return result;
  }

  outcome::result<GroupPacket> DrandSyncClientImpl::group() {
    ::drand::GroupRequest request;
    grpc::ClientContext context;
    ::drand::GroupPacket response;
    auto status = stub_->Group(&context, request, &response);
    if (not status.ok()) {
      return Error::kGRPCFailure;
    }
    return ProtoParser::protoToHandy(response);
  }
}  // namespace fc::drand
