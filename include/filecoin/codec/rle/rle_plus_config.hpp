/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CODEC_RLE_PLUS_CONFIG_HPP
#define CODEC_RLE_PLUS_CONFIG_HPP

namespace fc::codec::rle {
  /**
   * @var Num of bits to store small block value
   */
  const int SMALL_BLOCK_LENGTH = 0x04;

  /**
   * @var Bitwise shift size for pack block value
   */
  const int PACK_BYTE_SHIFT = 0x07;

  /**
   * @var Num of bits in the byte
   */
  const int BYTE_BITS_COUNT = 0x08;

  /**
   * @var Value of the long block
   */
  const int LONG_BLOCK_VALUE = 0x10;

  /**
   * @var Bitwise mask for unpack long block value
   */
  const int UNPACK_BYTE_MASK = 0x7F;

  /**
   * @var MSB value
   */
  const int BYTE_SLICE_VALUE = 0x80;

  /**
   * @var Maximum object size to encode or decode
   */
  const int OBJECT_MAX_SIZE = 0x100000;

};  // namespace fc::codec::rle

#endif
