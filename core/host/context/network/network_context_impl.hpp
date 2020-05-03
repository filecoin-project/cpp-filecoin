/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FUHON_CORE_HOST_CONTEXT_NETWORK_IMPL_HPP
#define CPP_FUHON_CORE_HOST_CONTEXT_NETWORK_IMPL_HPP

#include <vector>

#include <libp2p/muxer/mplex.hpp>
#include <libp2p/muxer/yamux.hpp>
#include <libp2p/network/impl/connection_manager_impl.hpp>
#include <libp2p/network/impl/dialer_impl.hpp>
#include <libp2p/network/impl/listener_manager_impl.hpp>
#include <libp2p/network/impl/network_impl.hpp>
#include <libp2p/network/impl/router_impl.hpp>
#include <libp2p/network/impl/transport_manager_impl.hpp>
#include <libp2p/peer/address_repository/inmem_address_repository.hpp>
#include <libp2p/peer/impl/peer_repository_impl.hpp>
#include <libp2p/peer/key_repository/inmem_key_repository.hpp>
#include <libp2p/peer/protocol_repository/inmem_protocol_repository.hpp>
#include <libp2p/protocol_muxer/multiselect.hpp>
#include <libp2p/transport/impl/upgrader_impl.hpp>
#include <libp2p/transport/tcp/tcp_transport.hpp>
#include "host/context/common_context.hpp"
#include "host/context/network_context.hpp"
#include "host/context/security_context.hpp"

namespace fc::host::context::network {
  class NetworkContextImpl : public NetworkContext {
   protected:
    using SecurityContext = security::SecurityContext;
    using CommonContext = common::CommonContext;
    using Multiselect = libp2p::protocol_muxer::Multiselect;
    using MuxedConnectionConfig = libp2p::muxer::MuxedConnectionConfig;
    using MuxerAdaptorSPtr = std::shared_ptr<libp2p::muxer::MuxerAdaptor>;
    using Mplex = libp2p::muxer::Mplex;
    using Yamux = libp2p::muxer::Yamux;
    using UpgraderImpl = libp2p::transport::UpgraderImpl;
    using TcpTransport = libp2p::transport::TcpTransport;
    using TransportSPtr = std::shared_ptr<libp2p::transport::TransportAdaptor>;
    using TransportManagerImpl = libp2p::network::TransportManagerImpl;
    using ConnectionManagerImpl = libp2p::network::ConnectionManagerImpl;
    using ListenManagerImpl = libp2p::network::ListenerManagerImpl;
    using RouterImpl = libp2p::network::RouterImpl;
    using Dialer = libp2p::network::Dialer;
    using DialerImpl = libp2p::network::DialerImpl;
    using Network = libp2p::network::Network;
    using NetworkImpl = libp2p::network::NetworkImpl;
    using PeerRepository = libp2p::peer::PeerRepository;
    using PeerRepositoryImpl = libp2p::peer::PeerRepositoryImpl;
    using InMemAddressRepository = libp2p::peer::InmemAddressRepository;
    using InMemKeyRepository = libp2p::peer::InmemKeyRepository;
    using InMemProtocolRepository = libp2p::peer::InmemProtocolRepository;

   public:
    NetworkContextImpl(std::shared_ptr<SecurityContext> security_ctx,
                       std::shared_ptr<CommonContext> common_ctx)
        : security_context_{std::move(security_ctx)},
          common_context_{std::move(common_ctx)},
          multiselect_{std::make_shared<Multiselect>()},
          muxer_adaptors_{std::make_shared<Mplex>(muxer_config_),
                          std::make_shared<Yamux>(muxer_config_)},
          upgrader_{std::make_shared<UpgraderImpl>(
              multiselect_,
              security_context_->getSecurityAdaptors(),
              muxer_adaptors_)},
          tcp_transport_{std::make_shared<TcpTransport>(
              common_context_->getInputOutput(), upgrader_)},
          transport_adaptors_{tcp_transport_},
          transport_manager_{
              std::make_shared<TransportManagerImpl>(transport_adaptors_)},
          router_{std::make_shared<RouterImpl>()},
          connection_manager_{std::make_shared<ConnectionManagerImpl>(
              common_context_->getEventBus(), transport_manager_)},
          listen_manager_{std::make_shared<ListenManagerImpl>(
              multiselect_, router_, transport_manager_, connection_manager_)},
          address_repository_{std::make_shared<InMemAddressRepository>()},
          key_repository_{std::make_shared<InMemKeyRepository>()},
          protocol_repository_{std::make_shared<InMemProtocolRepository>()} {}

    std::shared_ptr<Host> createHost() const override;

   private:
    std::shared_ptr<SecurityContext> security_context_;
    std::shared_ptr<CommonContext> common_context_;
    MuxedConnectionConfig muxer_config_;
    std::shared_ptr<Multiselect> multiselect_;
    std::vector<MuxerAdaptorSPtr> muxer_adaptors_;
    std::shared_ptr<UpgraderImpl> upgrader_;
    std::shared_ptr<TcpTransport> tcp_transport_;
    std::vector<TransportSPtr> transport_adaptors_;
    std::shared_ptr<TransportManagerImpl> transport_manager_;
    std::shared_ptr<RouterImpl> router_;
    std::shared_ptr<ConnectionManagerImpl> connection_manager_;
    std::shared_ptr<ListenManagerImpl> listen_manager_;
    std::shared_ptr<InMemAddressRepository> address_repository_;
    std::shared_ptr<InMemKeyRepository> key_repository_;
    std::shared_ptr<InMemProtocolRepository> protocol_repository_;
  };
}  // namespace fc::host::context::network

#endif
