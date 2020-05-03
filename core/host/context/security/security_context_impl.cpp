/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host/context/security/security_context_impl.hpp"

namespace fc::host::context::security {
  std::shared_ptr<SecurityContextImpl::IdentityManager>
  SecurityContextImpl::getIdentityManager() const {
    return identity_manager_;
  }

  const std::vector<SecurityContextImpl::SecurityAdaptorSPtr>
      &SecurityContextImpl::getSecurityAdaptors() const {
    return security_adaptors_;
  }
}  // namespace fc::host::context::security
