# Daemon Pal - Compact user-space tool/library for process management.
# Copyright (C) 2024 Fabrika Charvat s.r.o. All rights reserved.
# Developed by Partoska Laboratory team, <https://lab.partoska.com>
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# You can contact the author(s) via email at ask <at> partoska.com.
#

cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 99)
project(dpal VERSION 1.3.0 LANGUAGES C)

add_subdirectory(src/lib)
add_subdirectory(src/cli)

option(WITH_TESTS "Build tests" OFF)

if(WITH_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# CPack packaging (macOS and Linux only).
set(CPACK_PACKAGE_NAME "dpal")
set(CPACK_PACKAGE_VENDOR "Partoska Laboratory")
set(CPACK_PACKAGE_CONTACT "ask@partoska.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Compact user-space tool for process management")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/dist")

if(APPLE)
    set(CPACK_PACKAGE_FILE_NAME "dpal_${PROJECT_VERSION}_darwin_universal")
    if(POLICY CMP0161)
        cmake_policy(SET CMP0161 NEW)
    endif()
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cpack")
    set(CPACK_GENERATOR "productbuild;TGZ")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
    set(CPACK_COMMAND_PKGBUILD "${CMAKE_CURRENT_SOURCE_DIR}/cpack/pkgbuild_wrapper.sh")
    set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/assets/installer/WELCOME.macos.txt")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/assets/installer/README.macos.txt")
    # Installer background image, referenced by cpack/CPack.distribution.dist.in.
    set(CPACK_PRODUCTBUILD_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/installer/macos")
    set(CPACK_PRODUCTBUILD_IDENTIFIER "com.partoska.dpal")
    set(CPACK_PRODUCTBUILD_DOMAINS TRUE)
    set(CPACK_PRODUCTBUILD_DOMAINS_ANYWHERE TRUE)
    set(CPACK_PRODUCTBUILD_DOMAINS_USER FALSE)
    set(CPACK_PRODUCTBUILD_DOMAINS_ROOT TRUE)
elseif(UNIX)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CPACK_ARCH "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        set(CPACK_ARCH "armhf")
    else()
        set(CPACK_ARCH "amd64")
    endif()
    set(CPACK_PACKAGE_FILE_NAME "dpal_${PROJECT_VERSION}_linux_${CPACK_ARCH}")
    set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/assets/installer/WELCOME.linux.txt")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/assets/installer/README.linux.txt")
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_SECTION "admin")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/System")

    # Application icons for the freedesktop.org hicolor theme.
    set(DPAL_ICON_SIZES 16x16 24x24 32x32 48x48 64x64 128x128 256x256 512x512 scalable)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/hicolor" DESTINATION share/icons)
    # The icon theme directories belong to the system, not to this package.
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        /usr/share/icons
        /usr/share/icons/hicolor)
    foreach(size ${DPAL_ICON_SIZES})
        list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
            /usr/share/icons/hicolor/${size}
            /usr/share/icons/hicolor/${size}/apps)
    endforeach()
endif()

include(CPack)
