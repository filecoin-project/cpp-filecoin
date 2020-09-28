/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cgo.hpp"

#ifdef __cplusplus
extern "C" {
#endif

Raw gocRtIpldGet(Raw);
Raw gocRtIpldPut(Raw);
Raw gocRtCharge(Raw);
Raw gocRtRand(Raw);
Raw gocRtBlake(Raw);
Raw gocRtVerifyPost(Raw);
Raw gocRtVerifySeals(Raw);
Raw gocRtActorId(Raw);
Raw gocRtSend(Raw);
Raw gocRtVerifySig(Raw);
Raw gocRtCommD(Raw);
Raw gocRtNewAddress(Raw);
Raw gocRtCreateActor(Raw);
Raw gocRtActorCode(Raw);
Raw gocRtActorBalance(Raw);
Raw gocRtStateGet(Raw);
Raw gocRtStateCommit(Raw);
Raw gocRtDeleteActor(Raw);

#ifdef __cplusplus
}
#endif
