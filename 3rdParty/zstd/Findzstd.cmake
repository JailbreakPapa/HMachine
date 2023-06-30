#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# this file actually ingests the library and defines targets.
set(TARGET_WITH_NAMESPACE "3rdParty::zstd")
if (TARGET ${TARGET_WITH_NAMESPACE})
    return()
endif()

set(zstd_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/zstd/lib)
set(zstd_LIBS_DIR ${CMAKE_CURRENT_LIST_DIR}/zstd/bin)

if (${PAL_PLATFORM_NAME} STREQUAL "Android")
    set(zstd_ANDROID_BASE    ${zstd_LIBS_DIR}/android-21-arm64-v8a-clang-5.0)
    set(zstd_LIBRARY_DEBUG   ${zstd_ANDROID_BASE}/debug/libzstd.a)
    set(zstd_LIBRARY_RELEASE ${zstd_ANDROID_BASE}/release/libzstd.a)
elseif (${PAL_PLATFORM_NAME} STREQUAL "iOS")
    set(zstd_LIBRARY_DEBUG   ${zstd_LIBS_DIR}/ios-clang-703.0.31/debug/libzstd.a)
    set(zstd_LIBRARY_RELEASE ${zstd_LIBS_DIR}/ios-clang-703.0.31/release/libzstd.a)
elseif (${PAL_PLATFORM_NAME} STREQUAL "Linux")
    set(zstd_LIBRARY_DEBUG   ${zstd_LIBS_DIR}/linux-clang-3.8/debug/libzstd.a)
    set(zstd_LIBRARY_RELEASE ${zstd_LIBS_DIR}/linux-clang-3.8/release/libzstd.a)
elseif (${PAL_PLATFORM_NAME} STREQUAL "Mac")
    set(zstd_LIBRARY_DEBUG   ${zstd_LIBS_DIR}/darwin-clang-703.0.31/debug/libzstd.a)
    set(zstd_LIBRARY_RELEASE ${zstd_LIBS_DIR}/darwin-clang-703.0.31/release/libzstd.a)
elseif (${PAL_PLATFORM_NAME} STREQUAL "Windows")
    set(zstd_LIBRARY_DEBUG   ${zstd_LIBS_DIR}/win-x64-vc140/debug/zstd.lib)
    set(zstd_LIBRARY_RELEASE ${zstd_LIBS_DIR}/win-x64-vc140/release/zstd.lib)
endif()

# we set it to a generator expression for multi-config situations:
set(zstd_LIBRARY
    "$<$<CONFIG:profile>:${zstd_LIBRARY_RELEASE}>"
    "$<$<CONFIG:Release>:${zstd_LIBRARY_RELEASE}>"
    "$<$<CONFIG:Debug>:${zstd_LIBRARY_DEBUG}>")

add_library(${TARGET_WITH_NAMESPACE} INTERFACE IMPORTED GLOBAL)
ly_target_include_system_directories(TARGET ${TARGET_WITH_NAMESPACE} INTERFACE ${zstd_INCLUDE_DIR})
target_link_libraries(
    ${TARGET_WITH_NAMESPACE}
    INTERFACE
        ${zstd_LIBRARY}
)

set(zstd_FOUND True)