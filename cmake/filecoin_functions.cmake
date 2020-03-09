#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

include(GNUInstallDirs)

function(filecoin_add_library target)
  add_library(${target}
      ${ARGN}
      )
  filecoin_install(${target})
endfunction()
