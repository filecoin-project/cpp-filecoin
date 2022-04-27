/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include "vm/actor/builtin/types/verified_registry/remove_data_cap_request.hpp"

namespace fc::vm::actor::builtin::verifreg {
  using primitives::DataCap;
  using primitives::StoragePower;
  using types::verified_registry::RemoveDataCapRequest;

  // These methods must be actual with the last version of actors

  enum class VerifiedRegistryActor : MethodNumber {
    kConstruct = 1,
    kAddVerifier,
    kRemoveVerifier,
    kAddVerifiedClient,
    kUseBytes,
    kRestoreBytes,
    kRemoveVerifiedClientDataCap,  // since v7
  }

  struct Construct : ActorMethodBase<VerifiedRegistryActor::kConstruct> {
    using Params = Address;
  };

  struct AddVerifier : ActorMethodBase<VerifiedRegistryActor::kAddVerifier> {
    struct Params {
      Address address;
      DataCap allowance;

      inline bool operator==(const Params &other) const {
        return address == other.address && allowance == other.allowance;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(AddVerifier::Params, address, allowance)

  struct RemoveVerifier
      : ActorMethodBase<VerifiedRegistryActor::kRemoveVerifier> {
    using Params = Address;
  };

  struct AddVerifiedClient
      : ActorMethodBase<VerifiedRegistryActor::kAddVerifiedClient> {
    struct Params {
      Address address;
      DataCap allowance;

      inline bool operator==(const Params &other) const {
        return address == other.address && allowance == other.allowance;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(AddVerifiedClient::Params, address, allowance)

  struct UseBytes : ActorMethodBase<VerifiedRegistryActor::kUseBytes> {
    struct Params {
      Address address;
      StoragePower deal_size;

      inline bool operator==(const Params &other) const {
        return address == other.address && deal_size == other.deal_size;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(UseBytes::Params, address, deal_size)

  struct RestoreBytes : ActorMethodBase<VerifiedRegistryActor::kRestoreBytes> {
    struct Params {
      Address address;
      StoragePower deal_size;

      inline bool operator==(const Params &other) const {
        return address == other.address && deal_size == other.deal_size;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(RestoreBytes::Params, address, deal_size)

  struct RemoveVerifiedClientDataCap
      : ActorMethodBase<VerifiedRegistryActor::kRemoveVerifiedClientDataCap> {
    struct Params {
      Address client_to_remove;
      DataCap amount_to_remove;
      RemoveDataCapRequest request1;
      RemoveDataCapRequest request2;

      inline bool operator==(const Params &other) const {
        return client_to_remove == other.client_to_remove
               && amount_to_remove == other.amount_to_remove
               && request1 == other.request1 && request2 == other.request2;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };

    struct Result {
      Address verified_client;
      DataCap data_cap_removed;

      inline bool operator==(const Result &other) const {
        return verified_client == other.verified_client
               && data_cap_removed == other.data_cap_removed;
      }

      inline bool operator!=(const Result &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(RemoveVerifiedClientDataCap::Params,
             client_to_remove,
             amount_to_remove,
             request1,
             request2)
  CBOR_TUPLE(RemoveVerifiedClientDataCap::Result,
             verified_client,
             data_cap_removed)

}  // namespace fc::vm::actor::builtin::verifreg
