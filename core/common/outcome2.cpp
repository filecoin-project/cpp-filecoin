/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/outcome2.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc, OutcomeError, e) {
  return fc::enumStr(e);
}
