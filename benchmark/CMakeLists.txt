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

find_package(benchmark REQUIRED)
find_package(unofficial-brpc REQUIRED)
kmcmake_cc_bm(
        NAME pool_bench
        MODULE mem
        SOURCES
        pool_bench.cc
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
        NAME hash_map_bench
        MODULE mem
        SOURCES
        hash_map_bench.cc
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
        NAME resource_bench
        MODULE mem
        SOURCES
        resource_bench.cc
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
        NAME string_bench
        MODULE mem
        SOURCES
        string_bench.cc
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
        NAME vector_bench
        MODULE mem
        SOURCES
        vector_bench.cc
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
        NAME buffer_bench
        MODULE mem
        SOURCES
        buffer_bench.cc
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
        NAME buffer64_bench
        MODULE mem
        SOURCES
        buffer64_bench.cc
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
        NAME string64_bench
        MODULE mem
        SOURCES
        string64_bench.cc
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
        NAME vector_map_set_bench
        MODULE mem
        SOURCES
        vector_map_set_bench.cc
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
        NAME iobuf_benchmark
        MODULE mem
        SOURCES
        iobuf_benchmark.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        unofficial::brpc::brpc
        fermat::fermat_static
        benchmark::benchmark_main
        benchmark::benchmark
        ${KMCMAKE_DEPS_LINK}
)



kmcmake_cc_bm(
        NAME pri_queue_bench
        MODULE mem
        SOURCES
        pri_queue_bench.cc
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
        NAME stack_bench
        MODULE mem
        SOURCES
        stack_bench.cc
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
        NAME lru_cache
        MODULE mem
        SOURCES
        lru_cache.cc
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
        NAME sort_bench
        MODULE mem
        SOURCES
        sort_bench.cc
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

if(FERMAT_USE_MIMALLOC)
kmcmake_cc_bm(
        NAME malloc_bench
        MODULE mem
        SOURCES
        malloc_bench.cc
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
endif ()
kmcmake_cc_bm(
        NAME deque_bench
        MODULE mem
        SOURCES
        deque_bench.cc
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
        NAME list_bench
        MODULE mem
        SOURCES
        list_bench.cc
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
        NAME bitset_bench
        MODULE mem
        SOURCES
        bitset_bench.cc
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

find_package(fmt REQUIRED)

kmcmake_cc_bm(
        NAME stream_bench
        MODULE mem
        SOURCES
        stream_bench.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        fmt::fmt
        benchmark::benchmark_main
        benchmark::benchmark
        ${KMCMAKE_DEPS_LINK}
)



kmcmake_cc_bm(
        NAME reference_ptr_bench
        MODULE mem
        SOURCES
        reference_ptr_bench.cc
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


#[[
kmcmake_cc_bm(
        NAME find_char_bench
        MODULE mem
        SOURCES
        find_char_bench.cc
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
]]