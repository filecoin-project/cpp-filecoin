/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "cli/validate/with.hpp"

namespace fc::primitives::tipset {
 CLI_VALIDATE(Tipset) {
   return validateWith(out, values, [](const std::string &value) {
     // TODO string -> tipset
     /*
      * TipsetKey, это как cid.Cid[], только CbCid[], те парсить CID::fromString, и их TipsetKey::make
      * check lotus parsing in CLI method
      */
     return Tipset();
   });
 }
}  // namespace fc::primitives::address
