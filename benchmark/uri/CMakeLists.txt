#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#[[

kmcmake_cc_benchmark(
        NAME tedis_bench
        SOURCES tedis_bench.cc
        DEPS tedis::tedis ${KMCMAKE_DEPS_LINK} ${BENCHMARK_LIB} ${BENCHMARK_MAIN_LIB}
        COPTS ${USER_CXX_FLAGS}
)
]]

configure_file(${PROJECT_SOURCE_DIR}/benchmark/config.h.in ${PROJECT_SOURCE_DIR}/benchmark/config.h @ONLY)

find_package(simdjson REQUIRED)
find_package(uriparser REQUIRED)
kmcmake_cc_bm(
        NAME wpt_bench
        MODULE uri
        SOURCES
        wpt_bench.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        simdjson::simdjson
        fermat::fermat_static
        benchmark::benchmark_main
        benchmark::benchmark
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_bm(
        NAME percent_encode
        MODULE uri
        SOURCES
        percent_encode.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        uriparser::uriparser
        fermat::fermat_static
        benchmark::benchmark_main
        benchmark::benchmark
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_bm(
        NAME bbc_bench
        MODULE uri
        SOURCES
        bbc_bench.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        uriparser::uriparser
        fermat::fermat_static
        benchmark::benchmark_main
        benchmark::benchmark
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_bm(
        NAME bench
        MODULE uri
        SOURCES
        bench.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        uriparser::uriparser
        fermat::fermat_static
        benchmark::benchmark_main
        benchmark::benchmark
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_bm(
        NAME bench_search_params
        MODULE uri
        SOURCES
        bench_search_params.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        benchmark::benchmark_main
        benchmark::benchmark
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_bm(
        NAME model_bench
        MODULE uri
        SOURCES
        model_bench.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        benchmark::benchmark_main
        benchmark::benchmark
        ${KMCMAKE_DEPS_LINK}
)
