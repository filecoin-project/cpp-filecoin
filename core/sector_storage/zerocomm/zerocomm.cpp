/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/zerocomm/zerocomm.hpp"
#include "common/bitsutil.hpp"
#include "common/buffer.hpp"
#include "primitives/cid/comm_cid.hpp"

namespace fc::sector_storage::zerocomm {
  using common::Buffer;
  using common::countTrailingZeros;
  using primitives::cid::pieceCommitmentV1ToCID;

  const uint64_t kLevels = 37;
  const uint64_t kSkip = 2;
  const std::array<std::string, kLevels - kSkip> kPieceComms = {
      "3731bb99ac689f66eef5973e4a94da188f4ddcae580724fc6f3fd60dfd488333",
      "642a607ef886b004bf2c1978463ae1d4693ac0f410eb2d1b7a47fe205e5e750f",
      "57a2381a28652bf47f6bef7aca679be4aede5871ab5cf3eb2c08114488cb8526",
      "1f7ac9595510e09ea41c460b176430bb322cd6fb412ec57cb17d989a4310372f",
      "fc7e928296e516faade986b28f92d44a4f24b935485223376a799027bc18f833",
      "08c47b38ee13bc43f41b915c0eed9911a26086b3ed62401bf9d58b8d19dff624",
      "b2e47bfb11facd941f62af5c750f3ea5cc4df517d5c4f16db2b4d77baec1a32f",
      "f9226160c8f927bfdcc418cdf203493146008eaefb7d02194d5e548189005108",
      "2c1a964bb90b59ebfe0f6da29ad65ae3e417724a8f7c11745a40cac1e5e74011",
      "fee378cef16404b199ede0b13e11b624ff9d784fbbed878d83297e795e024f02",
      "8e9e2403fa884cf6237f60df25f83ee40dca9ed879eb6f6352d15084f5ad0d3f",
      "752d9693fa167524395476e317a98580f00947afb7a30540d625a9291cc12a07",
      "7022f60f7ef6adfa17117a52619e30cea82c68075adf1c667786ec506eef2d19",
      "d99887b973573a96e11393645236c17b1f4c7034d723c7a99f709bb4da61162b",
      "d0b530dbb0b4f25c5d2f2a28dfee808b53412a02931f18c499f5a254086b1326",
      "84c0421ba0685a01bf795a2344064fe424bd52a9d24377b394ff4c4b4568e811",
      "65f29e5d98d246c38b388cfc06db1f6b021303c5a289000bdce832a9c3ec421c",
      "a2247508285850965b7e334b3127b0c042b1d046dc54402137627cd8799ce13a",
      "dafdab6da9364453c26d33726b9fefe343be8f81649ec009aad3faff50617508",
      "d941d5e0d6314a995c33ffbd4fbe69118d73d4e5fd2cd31f0f7c86ebdd14e706",
      "514c435c3d04d349a5365fbd59ffc713629111785991c1a3c53af22079741a2f",
      "ad06853969d37d34ff08e09f56930a4ad19a89def60cbfee7e1d3381c1e71c37",
      "39560e7b13a93b07a243fd2720ffa7cb3e1d2e505ab3629e79f46313512cda06",
      "ccc3c012f5b05e811a2bbfdd0f6833b84275b47bf229c0052a82484f3c1a5b3d",
      "7df29b69773199e8f2b40b77919d048509eed768e2c7297b1f1437034fc3c62c",
      "66ce05a3667552cf45c02bcc4e8392919bdeac35de2ff56271848e9f7b675107",
      "d8610218425ab5e95b1ca6239d29a2e420d706a96f373e2f9c9a91d759d19b01",
      "6d364b1ef846441a5a4a68862314acc0a46f016717e53443e839eedf83c2853c",
      "077e5fde35c50a9303a55009e3498a4ebedff39c42b710b730d8ec7ac7afa63e",
      "e64005a6bfe3777953b8ad6ef93f0fca1049b2041654f2a411f7702799cece02",
      "259d3d6b1f4d876d1185e1123af6f5501af0f67cf15b5216255b7b178d12051d",
      "3f9a4d411da4ef1b36f35ff0a195ae392ab23fee7967b7c41b03d1613fc29239",
      "fe4ef328c61aa39cfdb2484eaa32a151b1fe3dfd1f96dd8c9711fd86d6c58113",
      "f55d68900e2d8381eccb8164cb9976f24b2de0dd61a31b97ce6eb23850d5e819",
      "aaaa8c4cb40aacee1e02dc65424b2a6c8e99f803b72f7929c4101d7fae6bff32"};

  outcome::result<CID> getZeroPieceCommitment(const UnpaddedPieceSize &size) {
    OUTCOME_TRY(
        comm,
        common::Buffer::fromHex(gsl::at(
            kPieceComms, countTrailingZeros(size.padded()) - kSkip - 5)));
    return pieceCommitmentV1ToCID(comm);
  }

}  // namespace fc::sector_storage::zerocomm
