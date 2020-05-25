#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

set(TEST_RESOURCES_PATH "${PROJECT_SOURCE_DIR}/test/testutil/resources")
configure_file(
    "${PROJECT_SOURCE_DIR}/test/testutil/resources/resources.hpp.in"
    "${PROJECT_SOURCE_DIR}/test/testutil/resources/resources.hpp"
)


