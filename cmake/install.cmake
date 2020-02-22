#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

include(GNUInstallDirs)

function (filecoin_install targets)
  install(TARGETS ${targets} EXPORT filecoinConfig
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX}
      )
endfunction()

install(
    EXPORT filecoinConfig
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/filecoin
    NAMESPACE filecoin::
)
