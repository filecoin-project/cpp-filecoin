/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_CODEC_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_CODEC_HPP

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include "common/outcome_throw.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::primitives::tipset {

  /**
   * @brief cbor-encodes Tipset instance
   * @tparam Stream cbor-encoder stream type
   * @param s stream reference
   * @param tipset Tipset instance
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Tipset &tipset) noexcept {
    return s << (s.list() << tipset.cids << tipset.blks << tipset.height);
  }

  /*
  func (t *ExpTipSet) UnmarshalCBOR(r io.Reader) error {
          br := cbg.GetPeeker(r)

          maj, extra, err := cbg.CborReadHeader(br)
          if err != nil {
                  return err
          }
          if maj != cbg.MajArray {
                  return fmt.Errorf("cbor input should be of type array")
          }

          if extra != 3 {
                  return fmt.Errorf("cbor input had wrong number of fields")
          }

          // t.Cids ([]cid.Cid) (slice)

          maj, extra, err = cbg.CborReadHeader(br)
          if err != nil {
                  return err
          }

          if extra > cbg.MaxLength {
                  return fmt.Errorf("t.Cids: array too large (%d)", extra)
          }

          if maj != cbg.MajArray {
                  return fmt.Errorf("expected cbor array")
          }
          if extra > 0 {
                  t.Cids = make([]cid.Cid, extra)
          }
          for i := 0; i < int(extra); i++ {

                  c, err := cbg.ReadCid(br)
                  if err != nil {
                          return xerrors.Errorf("reading cid field t.Cids
  failed: %w", err)
                  }
                  t.Cids[i] = c
          }

          // t.Blocks ([]*types.BlockHeader) (slice)

          maj, extra, err = cbg.CborReadHeader(br)
          if err != nil {
                  return err
          }

          if extra > cbg.MaxLength {
                  return fmt.Errorf("t.Blocks: array too large (%d)", extra)
          }

          if maj != cbg.MajArray {
                  return fmt.Errorf("expected cbor array")
          }
          if extra > 0 {
                  t.Blocks = make([]*BlockHeader, extra)
          }
          for i := 0; i < int(extra); i++ {

                  var v BlockHeader
                  if err := v.UnmarshalCBOR(br); err != nil {
                          return err
                  }

                  t.Blocks[i] = &v
          }

          // t.Height (uint64) (uint64)

          maj, extra, err = cbg.CborReadHeader(br)
          if err != nil {
                  return err
          }
          if maj != cbg.MajUnsignedInt {
                  return fmt.Errorf("wrong type for uint64 field")
          }
          t.Height = uint64(extra)
          return nil
  }
     */

  /**
   * @brief cbor-decodes Tipset instance
   * @tparam Stream cbor-decoder stream type
   * @param s stream reference
   * @param tipset Tipset instance reference to decode into
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Tipset &tipset) {
    std::vector<CID> cids;
    std::vector<boost::optional<block::BlockHeader>> blks;

    s.list() >> cids >> blks >> tipset.height;
    tipset.cids = std::move(cids);
    tipset.blks = std::move(blks);
    return s;
  }

}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_CODEC_HPP
