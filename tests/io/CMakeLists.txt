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
        NAME iobuf_test
        MODULE io
        SOURCES
        iobuf_test.cc
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
#[[
kmcmake_cc_test(
        NAME customer_test
        MODULE io
        SOURCES
        customer_test.cc
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
]]

kmcmake_cc_test(
        NAME lease_test
        MODULE io
        SOURCES
        lease_test.cc
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
        NAME advance_lease_test
        MODULE io
        SOURCES
        advance_lease_test.cc
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
        NAME append_test
        MODULE io
        SOURCES
        append_test.cc
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
        NAME status_test
        MODULE io
        SOURCES
        status_test.cc
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
        NAME bad_test
        MODULE io
        SOURCES
        bad_test.cc
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
        NAME aligned_test
        MODULE io
        SOURCES
        aligned_test.cc
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
        NAME resource_test
        MODULE io
        SOURCES
        resource_test.cc
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
        NAME flatten_test
        MODULE io
        SOURCES
        flatten_test.cc
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