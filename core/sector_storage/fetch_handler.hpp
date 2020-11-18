/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_FETCH_HANDLER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_FETCH_HANDLER_HPP

#include "api/rpc/ws.hpp"
#include "sector_storage/stores/store.hpp"

namespace fc::sector_storage {

  api::RouteHandler serveHttp(std::shared_ptr<stores::LocalStore> local_store);

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_FETCH_HANDLER_HPP
