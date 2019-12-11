/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_ACTOR_ACTOR_HPP
#define CPP_FILECOIN_CORE_ACTOR_ACTOR_HPP

#include <libp2p/multi/content_identifier_codec.hpp>

namespace fc::actor {
  using libp2p::multi::ContentIdentifier;

  bool isBuiltinActor(ContentIdentifier code);

  bool isSingletonActor(ContentIdentifier code);

  extern ContentIdentifier kEmptyObjectCid;

  extern ContentIdentifier kAccountCodeCid, kCronCodeCid, kStoragePowerCodeCid,
      kStorageMarketCodeCid, kStorageMinerCodeCid, kMultisigCodeCid,
      kInitCodeCid, kPaymentChannelCodeCid;
}  // namespace fc::actor

#endif  // CPP_FILECOIN_CORE_ACTOR_ACTOR_HPP
