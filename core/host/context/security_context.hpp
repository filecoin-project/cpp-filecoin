/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FUHON_CORE_HOST_CONTEXT_SECURITY_HPP
#define CPP_FUHON_CORE_HOST_CONTEXT_SECURITY_HPP

#include <memory>
#include <vector>

#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/security/security_adaptor.hpp>

namespace fc::host::context::security {
  class SecurityContext {
   protected:
    using IdentityManager = libp2p::peer::IdentityManager;
    using SecurityAdaptorSPtr = std::shared_ptr<libp2p::security::SecurityAdaptor>;

   public:
    virtual ~SecurityContext() = default;

    virtual std::shared_ptr<IdentityManager> getIdentityManager() const = 0;

    virtual const std::vector<SecurityAdaptorSPtr> &getSecurityAdaptors() const = 0;
  };
}  // namespace fc::host::context::security

#endif
