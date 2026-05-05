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
###################################################################################################
# kmcmake_cc_library
################################################################################################

################################################################################
# kmcmake_cc_library
#
# Creates both `<name>_static` and `<name>_shared` from the same object sources,
# and exposes aliases:
#   - <namespace>::<name>_static
#   - <namespace>::<name>  (shared alias)
#
# Main options:
#   PUBLIC          Install targets when enabled.
#   EXCLUDE_SYSTEM  Do not mark includes as SYSTEM.
#   UNITY           Enable unity build on generated object target.
#
# Main args:
#   NAME            Logical library name. Defaults to current folder name.
#   NAMESPACE       Alias namespace. Defaults to `${PROJECT_NAME}`.
#   SHARE           Per-library override for shared install behavior ("ON").
#
# Typical usage:
# kmcmake_cc_library(
#   PUBLIC
#   NAME mylib
#   NAMESPACE myproj
#   SOURCES a.cc b.cc
#   HEADERS a.h
#   INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include
#   PINCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src
#   LINKS Threads::Threads
# )
#
# Notes:
# - Build artifacts are always created for static/shared variants.
# - Installation behavior for shared libraries is controlled by:
#     global `KMCMAKE_ENABLE_SHARE` or per-target `SHARE ON`.
################################################################################
function(kmcmake_cc_library)
    set(options
            PUBLIC
            EXCLUDE_SYSTEM
            UNITY
    )
    set(args NAME
            NAMESPACE
            SHARE
    )

    set(list_args
            DEPS
            SOURCES
            OBJECTS
            HEADERS
            INCLUDES
            PINCLUDES
            DEFINES
            COPTS
            CXXOPTS
            CUOPTS
            LINKS
            PLINKS
            WLINKS
    )

    cmake_parse_arguments(
            PARSE_ARGV 0
            KMCMAKE_CC_LIB
            "${options}"
            "${args}"
            "${list_args}"
    )

    if ("${KMCMAKE_CC_LIB_NAME}" STREQUAL "")
        get_filename_component(KMCMAKE_CC_LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        string(REPLACE " " "_" KMCMAKE_CC_LIB_NAME ${KMCMAKE_CC_LIB_NAME})
        kmcmake_print(" Library, NAME argument not provided. Using folder name:  ${KMCMAKE_CC_LIB_NAME}")
    endif ()

    if ("${KMCMAKE_CC_LIB_NAMESPACE}" STREQUAL "")
        set(KMCMAKE_CC_LIB_NAMESPACE ${PROJECT_NAME})
        kmcmake_print(" Library, NAMESPACE argument not provided. Using target alias:  ${KMCMAKE_CC_LIB_NAME}::${KMCMAKE_CC_LIB_NAME}")
    endif ()

    # Shared install switch:
    # 1) default to global option, 2) allow per-library override via SHARE ON.
    set(__ENABLE_SHARE ${KMCMAKE_ENABLE_SHARE})
    if(KMCMAKE_CC_LIB_SHARE)
        if ("${KMCMAKE_CC_LIB_SHARE}" STREQUAL "ON")
            set(__ENABLE_SHARE ON)
        endif ()
    endif()



    kmcmake_raw("-----------------------------------")
    if (KMCMAKE_CC_LIB_PUBLIC)
        set(KMCMAKE_LIB_INFO "${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME}  SHARED&STATIC PUBLIC")
    else ()
        set(KMCMAKE_LIB_INFO "${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME}  SHARED&STATIC INTERNAL")
    endif ()

    set(${KMCMAKE_CC_LIB_NAME}_INCLUDE_SYSTEM SYSTEM)
    if (KMCMAKE_CC_LIB_EXCLUDE_SYSTEM)
        set(${KMCMAKE_CC_LIB_NAME}_INCLUDE_SYSTEM "")
    endif ()
    set(_KMCMAKE_CC_LIB_PUBLIC_INCLUDES
            ${KMCMAKE_CC_LIB_INCLUDES}
            "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${${PROJECT_NAME}_BINARY_DIR}>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )

    kmcmake_print_label("Create Library" "${KMCMAKE_LIB_INFO}")
    kmcmake_raw("-----------------------------------")
    if (VERBOSE_KMCMAKE_BUILD)
        kmcmake_print_list_label("Sources" KMCMAKE_CC_LIB_SOURCES)
        kmcmake_print_list_label("Objects" KMCMAKE_CC_LIB_OBJECTS)
        kmcmake_print_list_label("Deps" KMCMAKE_CC_LIB_DEPS)
        kmcmake_print_list_label("COPTS" KMCMAKE_CC_LIB_COPTS)
        kmcmake_print_list_label("CXXOPTS" KMCMAKE_CC_LIB_CXXOPTS)
        kmcmake_print_list_label("CUOPTS" KMCMAKE_CC_LIB_CUOPTS)
        kmcmake_print_list_label("Defines" KMCMAKE_CC_LIB_DEFINES)
        kmcmake_print_list_label("Includes" KMCMAKE_CC_LIB_INCLUDES)
        kmcmake_print_list_label("Private Includes" KMCMAKE_CC_LIB_PINCLUDES)
        kmcmake_print_list_label("Links" KMCMAKE_CC_LIB_LINKS)
        kmcmake_print_list_label("Private Links" KMCMAKE_CC_LIB_PLINKS)
        kmcmake_raw("-----------------------------------")
    endif ()
    set(KMCMAKE_CC_LIB_OBJECTS_FLATTEN)
    if (KMCMAKE_CC_LIB_OBJECTS)
        foreach (obj IN LISTS KMCMAKE_CC_LIB_OBJECTS)
            list(APPEND KMCMAKE_CC_LIB_OBJECTS_FLATTEN $<TARGET_OBJECTS:${obj}>)
        endforeach ()
    endif ()
    if (KMCMAKE_CC_LIB_SOURCES)
        # Compile sources once into object library, then reuse in static/shared.
        add_library(${KMCMAKE_CC_LIB_NAME}_OBJECT OBJECT ${KMCMAKE_CC_LIB_SOURCES} ${KMCMAKE_CC_LIB_HEADERS})
        list(APPEND KMCMAKE_CC_LIB_OBJECTS_FLATTEN $<TARGET_OBJECTS:${KMCMAKE_CC_LIB_NAME}_OBJECT>)
        if (KMCMAKE_CC_LIB_DEPS)
            add_dependencies(${KMCMAKE_CC_LIB_NAME}_OBJECT ${KMCMAKE_CC_LIB_DEPS})
        endif ()
        
        # Optional unity build for faster compilation on large source sets.
        if (KMCMAKE_CC_LIB_UNITY)
            set_target_properties(${KMCMAKE_CC_LIB_NAME}_OBJECT PROPERTIES
                    UNITY_BUILD ON
                    UNITY_BUILD_BATCH_SIZE 20
            )
        endif ()

        # Optional precompiled headers. User headers are appended when provided.
        if (KMCMAKE_CC_LIB_HEADERS)
            target_precompile_headers(${KMCMAKE_CC_LIB_NAME}_OBJECT PUBLIC
                    <vector>
                    <string>
                    ${KMCMAKE_CC_LIB_HEADERS}
            )
        endif ()


        set_property(TARGET ${KMCMAKE_CC_LIB_NAME}_OBJECT PROPERTY POSITION_INDEPENDENT_CODE 1)
        target_compile_options(${KMCMAKE_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:C>:${KMCMAKE_CC_LIB_COPTS}>)
        target_compile_options(${KMCMAKE_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${KMCMAKE_CC_LIB_CXXOPTS}>)
        target_compile_options(${KMCMAKE_CC_LIB_NAME}_OBJECT PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${KMCMAKE_CC_LIB_CUOPTS}>)
        target_include_directories(${KMCMAKE_CC_LIB_NAME}_OBJECT ${${KMCMAKE_CC_LIB_NAME}_INCLUDE_SYSTEM}
                PRIVATE
                ${_KMCMAKE_CC_LIB_PUBLIC_INCLUDES}
        )
        target_include_directories(${KMCMAKE_CC_LIB_NAME}_OBJECT ${${KMCMAKE_CC_LIB_NAME}_INCLUDE_SYSTEM}
                PRIVATE
                ${KMCMAKE_CC_LIB_PINCLUDES}
        )

        target_compile_definitions(${KMCMAKE_CC_LIB_NAME}_OBJECT
                PRIVATE
                ${KMCMAKE_CC_LIB_DEFINES}
        )
    endif ()

    list(LENGTH KMCMAKE_CC_LIB_OBJECTS_FLATTEN obj_len)
    if (obj_len EQUAL -1)
        kmcmake_error("no source or object give to the library ${KMCMAKE_CC_LIB_NAME}")
    endif ()
    add_library(${KMCMAKE_CC_LIB_NAME}_static STATIC ${KMCMAKE_CC_LIB_OBJECTS_FLATTEN})
    if (${KMCMAKE_CC_LIB_NAME}_OBJECT)
        add_dependencies(${KMCMAKE_CC_LIB_NAME}_static ${KMCMAKE_CC_LIB_NAME}_OBJECT)
    endif ()
    if (KMCMAKE_CC_LIB_DEPS)
        add_dependencies(${KMCMAKE_CC_LIB_NAME}_static ${KMCMAKE_CC_LIB_DEPS})
    endif ()
    target_link_libraries(${KMCMAKE_CC_LIB_NAME}_static PRIVATE ${KMCMAKE_CC_LIB_PLINKS})
    target_link_libraries(${KMCMAKE_CC_LIB_NAME}_static PUBLIC ${KMCMAKE_CC_LIB_LINKS})
    target_link_libraries(${KMCMAKE_CC_LIB_NAME}_static PRIVATE ${KMCMAKE_CC_LIB_WLINKS})
    target_compile_definitions(${KMCMAKE_CC_LIB_NAME}_static PUBLIC ${KMCMAKE_CC_LIB_DEFINES})
    set_target_properties(${KMCMAKE_CC_LIB_NAME}_static PROPERTIES
            INTERFACE_KMCMAKE_RUNTIME_SIMD_LEVEL "${KMCMAKE_RUNTIME_SIMD_LEVEL}"
            INTERFACE_KMCMAKE_ARCH_FLAGS "${KMCMAKE_ARCH_OPTION}"
            INTERFACE_KMCMAKE_CXX_OPTIONS "${KMCMAKE_CXX_OPTIONS}"
            KMCMAKE_RUNTIME_SIMD_LEVEL "${KMCMAKE_RUNTIME_SIMD_LEVEL}"
            KMCMAKE_ARCH_FLAGS "${KMCMAKE_ARCH_OPTION}"
            KMCMAKE_CXX_OPTIONS "${KMCMAKE_CXX_OPTIONS}"
            EXPORT_PROPERTIES "KMCMAKE_RUNTIME_SIMD_LEVEL;KMCMAKE_ARCH_FLAGS;KMCMAKE_CXX_OPTIONS"
    )
    set_target_properties(${KMCMAKE_CC_LIB_NAME}_static PROPERTIES
            OUTPUT_NAME ${KMCMAKE_CC_LIB_NAME})
    add_library(${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME}_static ALIAS ${KMCMAKE_CC_LIB_NAME}_static)
    target_include_directories(${KMCMAKE_CC_LIB_NAME}_static ${${KMCMAKE_CC_LIB_NAME}_INCLUDE_SYSTEM}
            PUBLIC
            ${_KMCMAKE_CC_LIB_PUBLIC_INCLUDES}
    )

    add_library(${KMCMAKE_CC_LIB_NAME}_shared SHARED ${KMCMAKE_CC_LIB_OBJECTS_FLATTEN})
    if (${KMCMAKE_CC_LIB_NAME}_OBJECT)
        add_dependencies(${KMCMAKE_CC_LIB_NAME}_shared ${KMCMAKE_CC_LIB_NAME}_OBJECT)
    endif ()
    if (KMCMAKE_CC_LIB_DEPS)
        add_dependencies(${KMCMAKE_CC_LIB_NAME}_shared ${KMCMAKE_CC_LIB_DEPS})
    endif ()
    target_link_libraries(${KMCMAKE_CC_LIB_NAME}_shared PRIVATE ${KMCMAKE_CC_LIB_PLINKS})
    target_link_libraries(${KMCMAKE_CC_LIB_NAME}_shared PUBLIC ${KMCMAKE_CC_LIB_LINKS})
    target_compile_definitions(${KMCMAKE_CC_LIB_NAME}_shared PUBLIC ${KMCMAKE_CC_LIB_DEFINES})
    set_target_properties(${KMCMAKE_CC_LIB_NAME}_shared PROPERTIES
            INTERFACE_KMCMAKE_RUNTIME_SIMD_LEVEL "${KMCMAKE_RUNTIME_SIMD_LEVEL}"
            INTERFACE_KMCMAKE_ARCH_FLAGS "${KMCMAKE_ARCH_OPTION}"
            INTERFACE_KMCMAKE_CXX_OPTIONS "${KMCMAKE_CXX_OPTIONS}"
            KMCMAKE_RUNTIME_SIMD_LEVEL "${KMCMAKE_RUNTIME_SIMD_LEVEL}"
            KMCMAKE_ARCH_FLAGS "${KMCMAKE_ARCH_OPTION}"
            KMCMAKE_CXX_OPTIONS "${KMCMAKE_CXX_OPTIONS}"
            EXPORT_PROPERTIES "KMCMAKE_RUNTIME_SIMD_LEVEL;KMCMAKE_ARCH_FLAGS;KMCMAKE_CXX_OPTIONS"
    )
    foreach (link ${KMCMAKE_CC_LIB_WLINKS})
        target_link_libraries(${KMCMAKE_CC_LIB_NAME}_shared PRIVATE $<LINK_LIBRARY:WHOLE_ARCHIVE,${link}>)
    endforeach ()
    set_target_properties(${KMCMAKE_CC_LIB_NAME}_shared PROPERTIES
            OUTPUT_NAME ${KMCMAKE_CC_LIB_NAME}
            VERSION ${${PROJECT_NAME}_VERSION}
            SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR})
    add_library(${KMCMAKE_CC_LIB_NAMESPACE}::${KMCMAKE_CC_LIB_NAME} ALIAS ${KMCMAKE_CC_LIB_NAME}_shared)
    target_include_directories(${KMCMAKE_CC_LIB_NAME}_shared ${${KMCMAKE_CC_LIB_NAME}_INCLUDE_SYSTEM}
            PUBLIC
            ${_KMCMAKE_CC_LIB_PUBLIC_INCLUDES}
    )

    # Install policy:
    # - PUBLIC: install static always
    # - shared install only when enabled by __ENABLE_SHARE
    if (KMCMAKE_CC_LIB_PUBLIC)
        if (__ENABLE_SHARE)
            install(TARGETS ${KMCMAKE_CC_LIB_NAME}_shared ${KMCMAKE_CC_LIB_NAME}_static
                EXPORT ${PROJECT_NAME}Targets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        else()
            install(TARGETS ${KMCMAKE_CC_LIB_NAME}_static
                EXPORT ${PROJECT_NAME}Targets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            )
        endif()
    endif ()

    foreach (arg IN LISTS KMCMAKE_CC_LIB_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed argument: ${arg}")
    endforeach ()

endfunction()