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

# force this into config mode, so that it uses the config files instead of module files.
set(mikkelsen_DIR ${CMAKE_CURRENT_LIST_DIR})
find_package(mikkelsen 1.0.0.4 REQUIRED CONFIG)
