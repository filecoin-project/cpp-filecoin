/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/ws.hpp"
#include "sector_storage/stores/store.hpp"

namespace fc::sector_storage {

  api::AuthRouteHandler serveHttp(
      const std::shared_ptr<stores::LocalStore> &local_store);

}  // namespace fc::sector_storage
