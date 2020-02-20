/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP

#include "codec/cbor/cbor.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/message/message.hpp"

namespace fc::vm::message {
  CBOR_TUPLE(UnsignedMessage,
             to,
             from,
             nonce,
             value,
             gasPrice,
             gasLimit,
             method,
             params)

  CBOR_TUPLE(SignedMessage, message, signature)
};  // namespace fc::vm::message

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP
