# Copyright (C) Kumo inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

kmcmake_cc_test(
        NAME object_pool_test
        MODULE mem
        SOURCES
        object_pool_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME malloc_test
        MODULE mem
        SOURCES
        malloc_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME allocator_test
        MODULE mem
        SOURCES
        allocator_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME aligned_allocator
        MODULE mem
        SOURCES
        aligned_allocator.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME simple_resource_pool
        MODULE mem
        SOURCES
        simple_resource_pool.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME resource_pool
        MODULE mem
        SOURCES
        resource_pool.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)
