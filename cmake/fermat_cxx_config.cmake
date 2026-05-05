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

list(APPEND KMCMAKE_CLANG_CL_FLAGS
        "/W3"
        "/DNOMINMAX"
        "/DWIN32_LEAN_AND_MEAN"
        "/D_CRT_SECURE_NO_WARNINGS"
        "/D_SCL_SECURE_NO_WARNINGS"
        "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
)

list(APPEND KMCMAKE_CLANG_CL_TEST_FLAGS
        "-Wno-c99-extensions"
        "-Wno-deprecated-declarations"
        "-Wno-missing-noreturn"
        "-Wno-missing-prototypes"
        "-Wno-missing-variable-declarations"
        "-Wno-null-conversion"
        "-Wno-shadow"
        "-Wno-shift-sign-overflow"
        "-Wno-sign-compare"
        "-Wno-unused-function"
        "-Wno-unused-member-function"
        "-Wno-unused-parameter"
        "-Wno-unused-private-field"
        "-Wno-unused-template"
        "-Wno-used-but-marked-unused"
        "-Wno-zero-as-null-pointer-constant"
        "-Wno-gnu-zero-variadic-macro-arguments"
)

list(APPEND KMCMAKE_GCC_FLAGS
        "-Wall"
        "-Wextra"
        "-Wno-cast-qual"
        "-Wconversion-null"
        "-Wformat-security"
        "-Woverlength-strings"
        "-Wpointer-arith"
        "-Wno-undef"
        "-Wunused-local-typedefs"
        "-Wunused-result"
        "-Wvarargs"
        "-Wno-attributes"
        "-Wno-implicit-fallthrough"
        "-Wno-unused-parameter"
        "-Wno-unused-function"
        "-Wwrite-strings"
        "-Wclass-memaccess"
        "-Wno-sign-compare"
        "-DNOMINMAX"
)

list(APPEND KMCMAKE_GCC_TEST_FLAGS
        "-Wno-conversion-null"
        "-Wno-deprecated-declarations"
        "-Wno-missing-declarations"
        "-Wno-sign-compare"
        "-Wno-undef"
        "-Wno-sign-compare"
        "-Wno-unused-function"
        "-Wno-unused-parameter"
        "-Wno-unused-private-field"
)

list(APPEND KMCMAKE_LLVM_FLAGS
        "-Wall"
        "-Wextra"
        "-Wno-cast-qual"
        "-Wno-conversion"
        "-Wno-sign-compare"
        "-Wfloat-overflow-conversion"
        "-Wfloat-zero-conversion"
        "-Wfor-loop-analysis"
        "-Wformat-security"
        "-Wgnu-redeclared-enum"
        "-Winfinite-recursion"
        "-Wliteral-conversion"
        "-Wmissing-declarations"
        "-Woverlength-strings"
        "-Wpointer-arith"
        "-Wself-assign"
        "-Wno-shadow"
        "-Wstring-conversion"
        "-Wtautological-overlap-compare"
        "-Wno-undef"
        "-Wuninitialized"
        "-Wunreachable-code"
        "-Wunused-comparison"
        "-Wunused-local-typedefs"
        "-Wunused-result"
        "-Wno-vla"
        "-Wwrite-strings"
        "-Wno-float-conversion"
        "-Wno-implicit-float-conversion"
        "-Wno-implicit-int-float-conversion"
        "-Wno-implicit-int-conversion"
        "-Wno-shorten-64-to-32"
        "-Wno-sign-conversion"
        "-Wno-unused-parameter"
        "-Wno-unused-function"
        "-DNOMINMAX"
)

list(APPEND KMCMAKE_LLVM_TEST_FLAGS
        "-Wno-c99-extensions"
        "-Wno-deprecated-declarations"
        "-Wno-missing-noreturn"
        "-Wno-missing-prototypes"
        "-Wno-missing-variable-declarations"
        "-Wno-null-conversion"
        "-Wno-shadow"
        "-Wno-undef"
        "-Wno-shift-sign-overflow"
        "-Wno-sign-compare"
        "-Wno-unused-function"
        "-Wno-unused-member-function"
        "-Wno-unused-parameter"
        "-Wno-unused-private-field"
        "-Wno-unused-template"
        "-Wno-sign-compare"
        "-Wno-unused-function"
        "-Wno-used-but-marked-unused"
        "-Wno-zero-as-null-pointer-constant"
        "-Wno-gnu-zero-variadic-macro-arguments"
)

list(APPEND KMCMAKE_MSVC_FLAGS
        "/W3"
        "/DNOMINMAX"
        "/DWIN32_LEAN_AND_MEAN"
        "/D_CRT_SECURE_NO_WARNINGS"
        "/D_SCL_SECURE_NO_WARNINGS"
        "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
        "/bigobj"
        "/wd4005"
        "/wd4068"
        "/wd4180"
        "/wd4244"
        "/wd4267"
        "/wd4503"
        "/wd4800"
)

list(APPEND KMCMAKE_MSVC_LINKOPTS
        "-ignore:4221"
)

list(APPEND KMCMAKE_MSVC_TEST_FLAGS
        "/wd4018"
        "/wd4101"
        "/wd4503"
        "/wd4996"
        "/DNOMINMAX"
)

list(APPEND KMCMAKE_RANDOM_HWAES_ARM32_FLAGS
        "-mfpu=neon"
)

list(APPEND KMCMAKE_RANDOM_HWAES_ARM64_FLAGS
        "-march=armv8-a+crypto"
)

list(APPEND KMCMAKE_RANDOM_HWAES_MSVC_X64_FLAGS
)

list(APPEND KMCMAKE_RANDOM_HWAES_X64_FLAGS
        "-maes"
        "-msse4.1"
)

################################################################################################
# cxx options
################################################################################################

set(KMCMAKE_LSAN_LINKOPTS "")
set(KMCMAKE_HAVE_LSAN OFF)
set(KMCMAKE_DEFAULT_LINKOPTS "")

if (BUILD_SHARED_LIBS AND MSVC)
    set(KMCMAKE_BUILD_DLL TRUE)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else ()
    set(KMCMAKE_BUILD_DLL FALSE)
endif ()

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
    if (MSVC)
        set(KMCMAKE_RANDOM_RANDEN_COPTS "${KMCMAKE_RANDOM_HWAES_MSVC_X64_FLAGS}")
    else ()
        set(KMCMAKE_RANDOM_RANDEN_COPTS "${KMCMAKE_RANDOM_HWAES_X64_FLAGS}")
    endif ()
elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm.*|aarch64")
    if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(KMCMAKE_RANDOM_RANDEN_COPTS "${KMCMAKE_RANDOM_HWAES_ARM64_FLAGS}")
    elseif ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(KMCMAKE_RANDOM_RANDEN_COPTS "${KMCMAKE_RANDOM_HWAES_ARM32_FLAGS}")
    else ()
        message(WARNING "Value of CMAKE_SIZEOF_VOID_P (${CMAKE_SIZEOF_VOID_P}) is not supported.")
    endif ()
else ()
    message(WARNING "Value of CMAKE_SYSTEM_PROCESSOR (${CMAKE_SYSTEM_PROCESSOR}) is unknown and cannot be used to set KMCMAKE_RANDOM_RANDEN_COPTS")
    set(KMCMAKE_RANDOM_RANDEN_COPTS "")
endif ()


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(KMCMAKE_DEFAULT_COPTS "${KMCMAKE_GCC_FLAGS}")
    set(KMCMAKE_TEST_COPTS "${KMCMAKE_GCC_FLAGS};${KMCMAKE_GCC_TEST_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # MATCHES so we get both Clang and AppleClang
    if (MSVC)
        # clang-cl is half MSVC, half LLVM
        set(KMCMAKE_DEFAULT_COPTS "${KMCMAKE_CLANG_CL_FLAGS}")
        set(KMCMAKE_TEST_COPTS "${KMCMAKE_CLANG_CL_FLAGS};${KMCMAKE_CLANG_CL_TEST_FLAGS}")
        set(KMCMAKE_DEFAULT_LINKOPTS "${KMCMAKE_MSVC_LINKOPTS}")
    else ()
        set(KMCMAKE_DEFAULT_COPTS "${KMCMAKE_LLVM_FLAGS}")
        set(KMCMAKE_TEST_COPTS "${KMCMAKE_LLVM_FLAGS};${KMCMAKE_LLVM_TEST_FLAGS}")
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            # AppleClang doesn't have lsan
            # https://developer.apple.com/documentation/code_diagnostics
            if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
                set(KMCMAKE_LSAN_LINKOPTS "-fsanitize=leak")
                set(KMCMAKE_HAVE_LSAN ON)
            endif ()
        endif ()
    endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(KMCMAKE_DEFAULT_COPTS "${KMCMAKE_MSVC_FLAGS}")
    set(KMCMAKE_TEST_COPTS "${KMCMAKE_MSVC_FLAGS};${KMCMAKE_MSVC_TEST_FLAGS}")
    set(KMCMAKE_DEFAULT_LINKOPTS "${KMCMAKE_MSVC_LINKOPTS}")
else ()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER}.  Building with no default flags")
    set(KMCMAKE_DEFAULT_COPTS "")
    set(KMCMAKE_TEST_COPTS "")
endif ()

##############################################################################
# default arch option
##############################################################################
set(KMCMAKE_ARCH_OPTION)

macro(kmcmake_apply_runtime_simd)
    # Reset generated macro values.
    set(KMCMAKE_SIMD_LEVEL_NONE_VAL 0)
    set(KMCMAKE_SIMD_LEVEL_SSE_VAL 0)
    set(KMCMAKE_SIMD_LEVEL_AVX_VAL 0)
    set(KMCMAKE_SIMD_LEVEL_AVX2_VAL 0)
    set(KMCMAKE_SIMD_LEVEL_BMI_VAL 0)
    set(KMCMAKE_SIMD_LEVEL_BMI2_VAL 0)
    set(KMCMAKE_SIMD_LEVEL_FMA_VAL 0)
    set(KMCMAKE_SIMD_LEVEL_MOVBE_VAL 0)

    if (NOT "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" AND NOT "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
        set(KMCMAKE_SIMD_LEVEL_NONE_VAL 1)
        return()
    endif ()

    if (NOT KMCMAKE_RUNTIME_SIMD_LEVEL)
        set(KMCMAKE_RUNTIME_SIMD_LEVEL "AVX2")
    endif ()
    string(TOUPPER "${KMCMAKE_RUNTIME_SIMD_LEVEL}" _KMCMAKE_RUNTIME_SIMD_LEVEL)

    if (_KMCMAKE_RUNTIME_SIMD_LEVEL STREQUAL "NONE")
        set(KMCMAKE_SIMD_LEVEL_NONE_VAL 1)
        return()
    endif ()

    set(_KMCMAKE_SIMD_LEVELS SSE SSE2 SSE3 SSSE3 SSE4_1 SSE4_2 AVX AVX2 AVX512)
    list(FIND _KMCMAKE_SIMD_LEVELS "${_KMCMAKE_RUNTIME_SIMD_LEVEL}" _KMCMAKE_SIMD_LEVEL_IDX)
    if (_KMCMAKE_SIMD_LEVEL_IDX EQUAL -1)
        kmcmake_error("Unsupported KMCMAKE_RUNTIME_SIMD_LEVEL=${_KMCMAKE_RUNTIME_SIMD_LEVEL}")
    endif ()

    # Base SSE chain.
    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 0 AND KMCMAKE_X86_SSE1)
        list(APPEND KMCMAKE_ARCH_OPTION ${SSE1_FLAG})
        set(KMCMAKE_SIMD_LEVEL_SSE_VAL 1)
    endif ()
    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 1 AND KMCMAKE_X86_SSE2)
        list(APPEND KMCMAKE_ARCH_OPTION ${SSE2_FLAG})
    endif ()
    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 2 AND KMCMAKE_X86_SSE3)
        list(APPEND KMCMAKE_ARCH_OPTION ${SSE3_FLAG})
    endif ()
    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 3 AND KMCMAKE_X86_SSSE3)
        list(APPEND KMCMAKE_ARCH_OPTION ${SSSE3_FLAG})
    endif ()
    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 4 AND KMCMAKE_X86_SSE4_1)
        list(APPEND KMCMAKE_ARCH_OPTION ${SSE4_1_FLAG})
    endif ()
    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 5 AND KMCMAKE_X86_SSE4_2)
        list(APPEND KMCMAKE_ARCH_OPTION ${SSE4_2_FLAG})
    endif ()

    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 6 AND KMCMAKE_X86_AVX)
        list(APPEND KMCMAKE_ARCH_OPTION ${AVX_FLAG})
        set(KMCMAKE_SIMD_LEVEL_AVX_VAL 1)
    endif ()

    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 7 AND KMCMAKE_X86_AVX2)
        list(APPEND KMCMAKE_ARCH_OPTION ${AVX2_FLAG})
        set(KMCMAKE_SIMD_LEVEL_AVX2_VAL 1)
        if (KMCMAKE_X86_BMI1)
            list(APPEND KMCMAKE_ARCH_OPTION ${BMI1_FLAG})
            set(KMCMAKE_SIMD_LEVEL_BMI_VAL 1)
        endif ()
        if (KMCMAKE_X86_BMI2)
            list(APPEND KMCMAKE_ARCH_OPTION ${BMI2_FLAG})
            set(KMCMAKE_SIMD_LEVEL_BMI2_VAL 1)
        endif ()
        if (KMCMAKE_X86_FMA)
            list(APPEND KMCMAKE_ARCH_OPTION ${FMA_FLAG})
            set(KMCMAKE_SIMD_LEVEL_FMA_VAL 1)
        endif ()
        if (KMCMAKE_X86_MOVBE)
            list(APPEND KMCMAKE_ARCH_OPTION ${MOVBE_FLAG})
            set(KMCMAKE_SIMD_LEVEL_MOVBE_VAL 1)
        endif ()
    endif ()

    if (_KMCMAKE_SIMD_LEVEL_IDX GREATER_EQUAL 8 AND KMCMAKE_X86_AVX512F)
        if (WIN32)
            list(APPEND KMCMAKE_ARCH_OPTION ${AVX512_FLAG})
        else ()
            list(APPEND KMCMAKE_ARCH_OPTION ${AVX512F_FLAG})
        endif ()
    endif ()

    if ("${KMCMAKE_ARCH_OPTION}" STREQUAL "")
        kmcmake_warn("Requested SIMD level ${_KMCMAKE_RUNTIME_SIMD_LEVEL} but no matching SIMD flags were detected. Falling back to generic build.")
        set(KMCMAKE_SIMD_LEVEL_NONE_VAL 1)
    endif ()
    list(REMOVE_DUPLICATES KMCMAKE_ARCH_OPTION)
endmacro()




#############################################################
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /DDEBUG" CACHE STRING "Debug mode flags for MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG" CACHE STRING "Release mode flags for MSVC")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /DNDEBUG" CACHE STRING "RelWithDebInfo mode flags for MSVC")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG" CACHE STRING "Debug mode flags for GCC/Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Release mode flags for GCC/Clang")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG" CACHE STRING "RelWithDebInfo mode flags for GCC/Clang")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()


kmcmake_apply_runtime_simd()



if (DEFINED ENV{KMCMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{KMCMAKE_CXX_FLAGS}")
endif ()

################################
# follow CC flag we provide
# ${KMCMAKE_DEFAULT_COPTS}
# ${KMCMAKE_TEST_COPTS}
# ${KMCMAKE_ARCH_OPTION} for arch option, by default, we set enable and
# ${KMCMAKE_RANDOM_RANDEN_COPTS}
# set it to haswell arch
##############################################################################
set(KMCMAKE_CXX_OPTIONS ${KMCMAKE_DEFAULT_COPTS} ${KMCMAKE_ARCH_OPTION} ${KMCMAKE_RANDOM_RANDEN_COPTS})
###############################
#
# define you options here
# eg.
# list(APPEND KMCMAKE_CXX_OPTIONS "-fopenmp")
list(REMOVE_DUPLICATES KMCMAKE_CXX_OPTIONS)
kmcmake_print_list_label("CXX_OPTIONS:" KMCMAKE_CXX_OPTIONS)