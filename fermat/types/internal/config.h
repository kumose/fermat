/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#pragma once


#ifndef FERMAT_EABASE_DISABLED
#include <fermat/base/eabase.h>
#endif

#if KM_TSAN_ENABLED
#include <sanitizer/tsan_interface.h>
#endif



///////////////////////////////////////////////////////////////////////////////
// KM_COMPILER_NO_STANDARD_CPP_LIBRARY
//
// Defined as 1 or undefined.
// Implements support for the definition of KM_COMPILER_NO_STANDARD_CPP_LIBRARY for the case
// of using EABase versions prior to the addition of its KM_COMPILER_NO_STANDARD_CPP_LIBRARY support.
//
#if !defined(KM_COMPILER_NO_STANDARD_CPP_LIBRARY)
#if defined(KM_PLATFORM_ANDROID)
// Disabled because EA's eaconfig/android_config/android_sdk packages currently
// don't support linking STL libraries. Perhaps we can figure out what linker arguments
// are needed for an app so we can manually specify them and then re-enable this code.
//
//#include <android/api-level.h>
//
//#if (__ANDROID_API__ < 9) // Earlier versions of Android provide no std C++ STL implementation.
#define KM_COMPILER_NO_STANDARD_CPP_LIBRARY 1
//#endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// EASTL namespace
//
// We define this so that users that #include this config file can reference
// these namespaces without seeing any other files that happen to use them.
///////////////////////////////////////////////////////////////////////////////

/// EA Standard Template Library
namespace fermat {
	// Intentionally empty.
}


///////////////////////////////////////////////////////////////////////////////
// FERMAT_DEBUG
//
// Defined as an integer >= 0. Default is 1 for debug builds and 0 for
// release builds. This define is also a master switch for the default value
// of some other settings.
//
// Example usage:
//    #if FERMAT_DEBUG
//       ...
//    #endif
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_DEBUG
#if defined(KM_DEBUG) || defined(_DEBUG)
#define FERMAT_DEBUG 1
#else
#define FERMAT_DEBUG 0
#endif
#endif

// Developer debug. Helps EASTL developers assert EASTL is coded correctly.
// Normally disabled for users since it validates internal things and not user things.
#ifndef FERMAT_DEV_DEBUG
#define FERMAT_DEV_DEBUG 0
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_DEBUGPARAMS_LEVEL
//
// FERMAT_DEBUGPARAMS_LEVEL controls what debug information is passed through to
// the allocator by default.
// This value may be defined by the user ... if not it will default to 1 for
// KM_DEBUG builds, otherwise 0.
//
//  0 - no debug information is passed through to allocator calls.
//  1 - 'name' is passed through to allocator calls.
//  2 - 'name', __FILE__, and __LINE__ are passed through to allocator calls.
//
// This parameter mirrors the equivalent parameter in the CoreAllocator package.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_DEBUGPARAMS_LEVEL
#if FERMAT_DEBUG
#define FERMAT_DEBUGPARAMS_LEVEL 2
#else
#define FERMAT_DEBUGPARAMS_LEVEL 0
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of KM_DLL.
// If KM_DLL is defined, then FERMAT_DLL is 1, else FERMAT_DLL is 0.
// KM_DLL is a define that controls DLL builds within the EAConfig build system.
// FERMAT_DLL controls whether EASTL is built and used as a DLL.
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef FERMAT_DLL
#if defined(KM_DLL)
#define FERMAT_DLL 1
#else
#define FERMAT_DLL 0
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_IF_NOT_DLL
//
// Utility to include expressions only for static builds.
//
#ifndef FERMAT_IF_NOT_DLL
#if FERMAT_DLL
#define FERMAT_IF_NOT_DLL(x)
#else
#define FERMAT_IF_NOT_DLL(x) x
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If KM_DLL is defined, then the user is building EASTL as a DLL and EASTL's
// non-templated functions will be exported. EASTL template functions are not
// labelled as FERMAT_API (and are thus not exported in a DLL build). This is
// because it's not possible (or at least unsafe) to implement inline templated
// functions in a DLL.
//
// Example usage of FERMAT_API:
//    FERMAT_API int someVariable = 10;      // Export someVariable in a DLL build.
//
//    struct FERMAT_API SomeClass{           // Export SomeClass and its member functions in a DLL build.
//        FERMAT_LOCAL void PrivateMethod(); // Not exported.
//    };
//
//    FERMAT_API void SomeFunction();        // Export SomeFunction in a DLL build.
//
//
#if defined(KM_DLL) && !defined(FERMAT_DLL)
#define FERMAT_DLL 1
#endif

#ifndef FERMAT_API // If the build file hasn't already defined this to be dllexport...
#if FERMAT_DLL
#define FERMAT_API      KM_IMPORT
#define FERMAT_LOCAL    KM_LOCAL
#else
#define FERMAT_API
#define FERMAT_LOCAL
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_EASTDC_API
//
// This is used for importing EAStdC functions into EASTL, possibly via a DLL import.
//
#ifndef FERMAT_EASTDC_API
#if FERMAT_DLL
#define FERMAT_EASTDC_API		KM_IMPORT
#define FERMAT_EASTDC_LOCAL		KM_LOCAL
#else
#define FERMAT_EASTDC_API
#define FERMAT_EASTDC_LOCAL
#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// FERMAT_NAME_ENABLED / FERMAT_NAME / FERMAT_NAME_VAL
//
// Used to wrap debug string names. In a release build, the definition
// goes away. These are present to avoid release build compiler warnings
// and to make code simpler.
//
// Example usage of FERMAT_NAME:
//    // pName will defined away in a release build and thus prevent compiler warnings.
//    void allocator::set_name(const char* FERMAT_NAME(pName))
//    {
//        #if FERMAT_NAME_ENABLED
//            mpName = pName;
//        #endif
//    }
//
// Example usage of FERMAT_NAME_VAL:
//    // "xxx" is defined to NULL in a release build.
//    vector<T, Allocator>::vector(const allocator_type& allocator = allocator_type(FERMAT_NAME_VAL("xxx")));
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_NAME_ENABLED
#define FERMAT_NAME_ENABLED FERMAT_DEBUG
#endif

#ifndef FERMAT_NAME
#if FERMAT_NAME_ENABLED
#define FERMAT_NAME(x)      x
#define FERMAT_NAME_VAL(x)  x
#else
#define FERMAT_NAME(x)
#define FERMAT_NAME_VAL(x) ((const char*)NULL)
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_DEFAULT_NAME_PREFIX
//
// Defined as a string literal. Defaults to "EASTL".
// This define is used as the default name for EASTL where such a thing is
// referenced in EASTL. For example, if the user doesn't specify an allocator
// name for their deque, it is named "EASTL deque". However, you can override
// this to say "SuperBaseball deque" by changing FERMAT_DEFAULT_NAME_PREFIX.
//
// Example usage (which is simply taken from how deque.h uses this define):
//     #ifndef FERMAT_DEQUE_DEFAULT_NAME
//         #define FERMAT_DEQUE_DEFAULT_NAME   FERMAT_DEFAULT_NAME_PREFIX " deque"
//     #endif
//
#ifndef FERMAT_DEFAULT_NAME_PREFIX
#define FERMAT_DEFAULT_NAME_PREFIX "EASTL"
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_ASSERT_ENABLED
//
// Defined as 0 or non-zero. Default is same as FERMAT_DEBUG.
// If FERMAT_ASSERT_ENABLED is non-zero, then asserts will be executed via
// the assertion mechanism.
//
// Example usage:
//     #if FERMAT_ASSERT_ENABLED
//         FERMAT_ASSERT(v.size() > 17);
//     #endif
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_ASSERT_ENABLED
#define FERMAT_ASSERT_ENABLED FERMAT_DEBUG
#endif

// Developer assert. Helps EASTL developers assert EASTL is coded correctly.
// Normally disabled for users since it validates internal things and not user things.
#ifndef FERMAT_DEV_ASSERT_ENABLED
#define FERMAT_DEV_ASSERT_ENABLED FERMAT_DEV_DEBUG
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
//
// Defined as 0 or non-zero. Default is same as FERMAT_ASSERT_ENABLED.
// This is like FERMAT_ASSERT_ENABLED, except it fires asserts specifically for
// a container operation that returns a reference while the container is empty.
// Sometimes people like to be able to take a reference to the front of the
// container, but won't use it if the container is empty. This may or may not
// be undefined behaviour depending on the container.
//
// In practice, for expressions such as &vector[0] this is not an issue -
// at least if the subscript operator is inlined because the expression will
// be equivalent to &*(nullptr) and optimized ti nullptr. MSVC, Clang and GCC
// all have this behaviour and UBSan & ASan report no issues with that code.
//
// Code that relies on this macro being disabled should instead use the
// container's data() member function. The range [data(), data() + size())
// is always valid, even when the container is empty (in which case data()
// is not dereferencable).
//
// Enabling this macro adds asserts if the container is empty and the function
// invocation is well defined. If the implementation may invoke UB, or the
// container is non-empty, then the assert fires if FERMAT_ASSERT_ENABLED is
// enabled, regardless of this macro.
//
// NOTE: If this is enabled, FERMAT_ASSERT_ENABLED must also be enabled to
// have any effect.
//
// Example usage:
//     template <typename T, typename Allocator>
//     inline typename vector<T, Allocator>::reference
//     vector<T, Allocator>::front()
//     {
//         #if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
//             FERMAT_ASSERT(mpEnd > mpBegin);
//         #endif
//
//         return *mpBegin;
//     }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
#define FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED FERMAT_ASSERT_ENABLED
#endif


///////////////////////////////////////////////////////////////////////////////
// SetAssertionFailureFunction
//
// Allows the user to set a custom assertion failure mechanism.
//
// Example usage:
//     void Assert(const char* pExpression, void* pContext);
//     SetAssertionFailureFunction(Assert, this);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_ASSERTION_FAILURE_DEFINED
#define FERMAT_ASSERTION_FAILURE_DEFINED

namespace fermat {
	using FERMAT_AssertionFailureFunction = void (*)(const char *pExpression, void *pContext);
	FERMAT_API void SetAssertionFailureFunction(FERMAT_AssertionFailureFunction pFunction, void *pContext);

	using FERMAT_AssertionFailureFunctionEx = void (*)(void *, const char *pExpression, void *pContext);
	FERMAT_API void SetAssertionFailureFunction(FERMAT_AssertionFailureFunctionEx pFunction, void *pContext);

	// These are the internal default functions that implement asserts.
	FERMAT_API void AssertionFailure(const char *pExpression);

	FERMAT_API void AssertionFailureFunctionDefault(const char *pExpression, void *pContext);

	FERMAT_API void AssertionFailure(void *instructionPointer, const char *pExpression);
}
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_ASSERT
//
// Assertion macro. Can be overridden by user with a different value.
//
// Example usage:
//    FERMAT_ASSERT(intVector.size() < 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_ASSERT
#if FERMAT_ASSERT_ENABLED
#define FERMAT_ASSERT(expression) \
			KM_DISABLE_VC_WARNING(4127) \
			do { \
				KM_ANALYSIS_ASSUME(expression); \
				(void)((expression) || (fermat::AssertionFailure(KM_GET_INSTRUCTION_POINTER(), #expression), 0)); \
			} while (0) \
			KM_RESTORE_VC_WARNING()
#else
#define FERMAT_ASSERT(expression)
#endif
#endif

// Developer assert. Helps EASTL developers assert EASTL is coded correctly.
// Normally disabled for users since it validates internal things and not user things.
#ifndef FERMAT_DEV_ASSERT
#if FERMAT_DEV_ASSERT_ENABLED
#define FERMAT_DEV_ASSERT(expression) \
			KM_DISABLE_VC_WARNING(4127) \
			do { \
				KM_ANALYSIS_ASSUME(expression); \
				(void)((expression) || (fermat::AssertionFailure(KM_GET_INSTRUCTION_POINTER(), #expression), 0)); \
			} while(0) \
			KM_RESTORE_VC_WARNING()
#else
#define FERMAT_DEV_ASSERT(expression)
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_ASSERT_MSG
//
// Example usage:
//    FERMAT_ASSERT_MSG(false, "detected error condition!");
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FERMAT_ASSERT_MSG
#if FERMAT_ASSERT_ENABLED
#define FERMAT_ASSERT_MSG(expression, message) \
			KM_DISABLE_VC_WARNING(4127) \
			do { \
				KM_ANALYSIS_ASSUME(expression); \
				(void)((expression) || (fermat::AssertionFailure(KM_GET_INSTRUCTION_POINTER(), message), 0)); \
			} while (0) \
			KM_RESTORE_VC_WARNING()
#else
#define FERMAT_ASSERT_MSG(expression, message)
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_FAIL_MSG
//
// Failure macro. Can be overridden by user with a different value.
//
// Example usage:
//    FERMAT_FAIL("detected error condition!");
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_FAIL_MSG
#if FERMAT_ASSERT_ENABLED
#define FERMAT_FAIL_MSG(message) (fermat::AssertionFailure(KM_GET_INSTRUCTION_POINTER(), message))
#else
#define FERMAT_FAIL_MSG(message)
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_CT_ASSERT / FERMAT_CT_ASSERT_NAMED
//
// FERMAT_CT_ASSERT is a macro for compile time assertion checks, useful for
// validating *constant* expressions. The advantage over using FERMAT_ASSERT
// is that errors are caught at compile time instead of runtime.
//
// Example usage:
//     FERMAT_CT_ASSERT(sizeof(uint32_t) == 4);
//
///////////////////////////////////////////////////////////////////////////////

#define FERMAT_CT_ASSERT(expression) static_assert(expression, #expression)


///////////////////////////////////////////////////////////////////////////////
// FERMAT_CT_ASSERT_MSG
//
// FERMAT_CT_ASSERT_MSG is a macro for compile time assertion checks, useful for
// validating *constant* expressions. The advantage over using FERMAT_ASSERT
// is that errors are caught at compile time instead of runtime.
// The message must be a string literal.
//
// Example usage:
//     FERMAT_CT_ASSERT_MSG(sizeof(uint32_t) == 4, "The size of uint32_t must be 4.");
//
///////////////////////////////////////////////////////////////////////////////

#define FERMAT_CT_ASSERT_MSG(expression, message) static_assert(expression, message)


///////////////////////////////////////////////////////////////////////////////
// FERMAT_DEBUG_BREAK / FERMAT_DEBUG_BREAK_OVERRIDE
//
// This function causes an app to immediately stop under the debugger.
// It is implemented as a macro in order to allow stopping at the site
// of the call.
//
// FERMAT_DEBUG_BREAK_OVERRIDE allows one to define FERMAT_DEBUG_BREAK directly.
// This is useful in cases where you desire to disable FERMAT_DEBUG_BREAK
// but do not wish to (or cannot) define a custom void function() to replace
// FERMAT_DEBUG_BREAK callsites.
//
// Example usage:
//     FERMAT_DEBUG_BREAK();
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_DEBUG_BREAK_OVERRIDE
#ifndef FERMAT_DEBUG_BREAK
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define FERMAT_DEBUG_BREAK() __debugbreak()    // This is a compiler intrinsic which will map to appropriate inlined asm for the platform.

#elif defined(KM_PLATFORM_NINTENDO)
#define FERMAT_DEBUG_BREAK() __builtin_debugtrap()  // Consider using the CLANG define
#elif (defined(KM_PROCESSOR_ARM) && !defined(KM_PROCESSOR_ARM64)) && defined(__APPLE__)
#define FERMAT_DEBUG_BREAK() asm("trap")
#elif defined(KM_PROCESSOR_ARM64) && defined(__APPLE__)
#include <signal.h>
#include <unistd.h>
#define FERMAT_DEBUG_BREAK() kill( getpid(), SIGINT )
#elif defined(KM_PROCESSOR_ARM64) && defined(__GNUC__)
#define FERMAT_DEBUG_BREAK() asm("brk 10")
#elif defined(KM_PROCESSOR_ARM) && defined(__GNUC__)
#define FERMAT_DEBUG_BREAK() asm("BKPT 10")     // The 10 is arbitrary. It's just a unique id.
#elif defined(KM_PROCESSOR_ARM) && defined(__ARMCC_VERSION)
#define FERMAT_DEBUG_BREAK() __breakpoint(10)
#elif defined(KM_PROCESSOR_POWERPC)               // Generic PowerPC.
#define FERMAT_DEBUG_BREAK() asm(".long 0")    // This triggers an exception by executing opcode 0x00000000.
#elif (defined(KM_PROCESSOR_X86) || defined(KM_PROCESSOR_X86_64)) && defined(KM_ASM_STYLE_INTEL)
#define FERMAT_DEBUG_BREAK() { __asm int 3 }
#elif (defined(KM_PROCESSOR_X86) || defined(KM_PROCESSOR_X86_64)) && (defined(KM_ASM_STYLE_ATT) || defined(__GNUC__))
#define FERMAT_DEBUG_BREAK() asm("int3")
#else
void FERMAT_DEBUG_BREAK(); // User must define this externally.
#endif
#else
void FERMAT_DEBUG_BREAK(); // User must define this externally.
#endif
#else
#ifndef FERMAT_DEBUG_BREAK
#if FERMAT_DEBUG_BREAK_OVERRIDE == 1
// define an empty callable to satisfy the call site.
#define FERMAT_DEBUG_BREAK ([]{})
#else
#define FERMAT_DEBUG_BREAK FERMAT_DEBUG_BREAK_OVERRIDE
#endif
#else
#error FERMAT_DEBUG_BREAK is already defined yet you would like to override it. Please ensure no other headers are already defining FERMAT_DEBUG_BREAK before this header (config.h) is included
#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// FERMAT_ALLOCATOR_COPY_ENABLED
//
// Defined as 0 or 1. Default is 0 (disabled) until some future date.
// If enabled (1) then container operator= copies the allocator from the
// source container. It ideally should be set to enabled but for backwards
// compatibility with older versions of EASTL it is currently set to 0.
// Regardless of whether this value is 0 or 1, this container copy constructs
// or copy assigns allocators.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_ALLOCATOR_COPY_ENABLED
#define FERMAT_ALLOCATOR_COPY_ENABLED 0
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_FIXED_SIZE_TRACKING_ENABLED
//
// Defined as an integer >= 0. Default is same as FERMAT_DEBUG.
// If FERMAT_FIXED_SIZE_TRACKING_ENABLED is enabled, then fixed
// containers in debug builds track the max count of objects
// that have been in the container. This allows for the tuning
// of fixed container sizes to their minimum required size.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_FIXED_SIZE_TRACKING_ENABLED
#define FERMAT_FIXED_SIZE_TRACKING_ENABLED FERMAT_DEBUG
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_EXCEPTIONS_ENABLED
//
// Defined as 0 or 1. Default is to follow what the compiler settings are.
// The user can predefine FERMAT_EXCEPTIONS_ENABLED to 0 or 1; however, if the
// compiler is set to disable exceptions then FERMAT_EXCEPTIONS_ENABLED is
// forced to a value of 0 regardless of the user predefine.
//
// Note that we do not enable EASTL exceptions by default if the compiler
// has exceptions enabled. To enable FERMAT_EXCEPTIONS_ENABLED you need to
// manually set it to 1.
//
///////////////////////////////////////////////////////////////////////////////

#if !defined(FERMAT_EXCEPTIONS_ENABLED) || ((FERMAT_EXCEPTIONS_ENABLED == 1) && defined(KM_COMPILER_NO_EXCEPTIONS))
#define FERMAT_EXCEPTIONS_ENABLED 0
#endif

///////////////////////////////////////////////////////////////////////////////
/// FERMAT_THROW_OR_ASSERT(exceptionType, message)
//
/// Throw an exception if exceptions enabled or assert if asserts are enabled,
/// otherwise no-op.
//
///////////////////////////////////////////////////////////////////////////////
#if FERMAT_EXCEPTIONS_ENABLED
#define FERMAT_THROW_OR_ASSERT(exceptionType, message) throw exceptionType()
#define FERMAT_THROW_MSG_OR_ASSERT(exceptionType, message) throw exceptionType(message)
#elif FERMAT_ASSERT_ENABLED
#define FERMAT_THROW_OR_ASSERT(exceptionType, message) FERMAT_FAIL_MSG(message)
#define FERMAT_THROW_MSG_OR_ASSERT(exceptionType, message) FERMAT_FAIL_MSG(message)
#else
// empty braces to prevent the following warning in the following context:
// if(expr)
//     FERMAT_THROW_OR_ASSERT(exceptionType, message);
// warning C4390: ';': empty controlled statement found; is this the intent?
#define FERMAT_THROW_OR_ASSERT(exceptionType, message) {}
#define FERMAT_THROW_MSG_OR_ASSERT(exceptionType, message) {}
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_INT128_SUPPORTED
//
// Defined as 0 or 1.
//
#ifndef FERMAT_INT128_SUPPORTED
#if defined(KM_COMPILER_INTMAX_SIZE) && (KM_COMPILER_INTMAX_SIZE >= 16)
#define FERMAT_INT128_SUPPORTED 1
#else
#define FERMAT_INT128_SUPPORTED 0
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_GCC_STYLE_INT128_SUPPORTED
//
// Defined as 0 or 1.
// Specifies whether __int128_t/__uint128_t are defined.
//
#ifndef FERMAT_GCC_STYLE_INT128_SUPPORTED
#if FERMAT_INT128_SUPPORTED && (defined(KM_COMPILER_GNUC) || defined(__clang__))
#define FERMAT_GCC_STYLE_INT128_SUPPORTED 1
#else
#define FERMAT_GCC_STYLE_INT128_SUPPORTED 0
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_INT128_DEFINED
//
// Defined as 0 or 1.
// Specifies whether fermat_int128_t/fermat_uint128_t have been typedef'd yet.
// NB: these types are not considered fundamental, arithmetic or integral when using the EAStdC implementation.
// this changes the compiler type traits defined in type_traits.h.
// eg. is_signed<fermat_int128_t>::value may be false, because it is not arithmetic.
//
#ifndef FERMAT_INT128_DEFINED
#if FERMAT_INT128_SUPPORTED
#define FERMAT_INT128_DEFINED 1

#if FERMAT_GCC_STYLE_INT128_SUPPORTED
typedef __int128_t fermat_int128_t;
typedef __uint128_t fermat_uint128_t;
#else
typedef int128_t fermat_int128_t; // The EAStdC package defines an EA::StdC::int128_t and uint128_t type,
typedef uint128_t fermat_uint128_t; // though they are currently within the EA::StdC namespace.
#endif
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_BITSET_WORD_TYPE_DEFAULT / FERMAT_BITSET_WORD_SIZE_DEFAULT
//
// Defined as an integral power of two type, usually uint32_t or uint64_t.
// Specifies the word type that bitset should use internally to implement
// storage. By default this is the platform register word size, but there
// may be reasons to use a different value.
//
// Defines the integral data type used by bitset by default.
// You can override this default on a bitset-by-bitset case by supplying a
// custom bitset WordType template parameter.
//
// The C++ standard specifies that the std::bitset word type be unsigned long,
// but that isn't necessarily the most efficient data type for the given platform.
// We can follow the standard and be potentially less efficient or we can do what
// is more efficient but less like the C++ std::bitset.
//
#if !defined(FERMAT_BITSET_WORD_TYPE_DEFAULT)
#if defined(FERMAT_BITSET_WORD_SIZE)         // FERMAT_BITSET_WORD_SIZE is deprecated, but we temporarily support the ability for the user to specify it. Use FERMAT_BITSET_WORD_TYPE_DEFAULT instead.

#if (FERMAT_BITSET_WORD_SIZE == 4)
#define FERMAT_BITSET_WORD_TYPE_DEFAULT uint32_t
#define FERMAT_BITSET_WORD_SIZE_DEFAULT 4
#else
#define FERMAT_BITSET_WORD_TYPE_DEFAULT uint64_t
#define FERMAT_BITSET_WORD_SIZE_DEFAULT 8
#endif
#elif (KM_PLATFORM_WORD_SIZE == 16)                     // KM_PLATFORM_WORD_SIZE is defined in EABase.
#define FERMAT_BITSET_WORD_TYPE_DEFAULT uint128_t
#define FERMAT_BITSET_WORD_SIZE_DEFAULT 16
#elif (KM_PLATFORM_WORD_SIZE == 8)
#define FERMAT_BITSET_WORD_TYPE_DEFAULT uint64_t
#define FERMAT_BITSET_WORD_SIZE_DEFAULT 8
#elif (KM_PLATFORM_WORD_SIZE == 4)
#define FERMAT_BITSET_WORD_TYPE_DEFAULT uint32_t
#define FERMAT_BITSET_WORD_SIZE_DEFAULT 4
#else
#define FERMAT_BITSET_WORD_TYPE_DEFAULT uint16_t
#define FERMAT_BITSET_WORD_SIZE_DEFAULT 2
#endif
#endif

#ifndef FERMAT_SLIST_SIZE_CACHE
#define FERMAT_SLIST_SIZE_CACHE 1
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_STD_ITERATOR_CATEGORY_ENABLED
//
// Defined as 0 or 1. Default is 0.
// If defined as non-zero, EASTL iterator categories (iterator.h's input_iterator_tag,
// forward_iterator_tag, etc.) are defined to be those from std C++ in the std
// namespace. The reason for wanting to enable such a feature is that it allows
// EASTL containers and algorithms to work with std STL containes and algorithms.
// The default value was changed from 1 to 0 in EASL 1.13.03, January 11, 2012.
// The reason for the change was that almost nobody was taking advantage of it and
// it was slowing down compile times for some compilers quite a bit due to them
// having a lot of headers behind <iterator>.
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_STD_ITERATOR_CATEGORY_ENABLED
#define FERMAT_STD_ITERATOR_CATEGORY_ENABLED 0
#endif

///////////////////////////////////////////////////////////////////////////////
// FERMAT_ITC_NS
//
// Deprecated. Was intended to be used as the namespace qualifier for iterator tags.
// Can now always use fermat as the namespace, as fermat will alias the iterator tags
// to the standard when FERMAT_STD_ITERATOR_CATEGORY_ENABLED == 1.
#if FERMAT_STD_ITERATOR_CATEGORY_ENABLED
#define FERMAT_ITC_NS std
#else
#define FERMAT_ITC_NS fermat
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_VALIDATION_ENABLED
//
// Defined as an integer >= 0. Default is to be equal to FERMAT_DEBUG.
// If nonzero, then a certain amount of automatic runtime validation is done.
// Runtime validation is not considered the same thing as asserting that user
// input values are valid. Validation refers to internal consistency checking
// of the validity of containers and their iterators. Validation checking is
// something that often involves significantly more than basic assertion
// checking, and it may sometimes be desirable to disable it.
//
// Validation sub-features are supported and can be enabled / disabled
// individually.
//
// This macro would generally be used internally by EASTL.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_VALIDATION_ENABLED
#define FERMAT_VALIDATION_ENABLED FERMAT_DEBUG
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_VALIDATE_COMPARE
//
// Defined as FERMAT_ASSERT or defined away. Default is FERMAT_ASSERT if FERMAT_VALIDATION_ENABLED is enabled.
// This is used to validate user-supplied comparison functions, particularly for sorting purposes.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_VALIDATE_COMPARE_ENABLED
#define FERMAT_VALIDATE_COMPARE_ENABLED FERMAT_VALIDATION_ENABLED
#endif

#if FERMAT_VALIDATE_COMPARE_ENABLED
#define FERMAT_VALIDATE_COMPARE(expression) FERMAT_ASSERT_MSG(expression, "Make sure the comparator satisfies the Compare named requirement (https://en.cppreference.com/w/cpp/named_req/Compare)")
#else
#define FERMAT_VALIDATE_COMPARE(expression)
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_VALIDATE_INTRUSIVE_LIST
//
// Defined as an integral value >= 0. Controls the amount of automatic validation
// done by intrusive_list. A value of 0 means no automatic validation is done.
// As of this writing, FERMAT_VALIDATE_INTRUSIVE_LIST defaults to 0, as it makes
// the intrusive_list_node become a non-POD, which may be an issue for some code.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_VALIDATE_INTRUSIVE_LIST
#define FERMAT_VALIDATE_INTRUSIVE_LIST 0
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_FORCE_INLINE
//
// Defined as a "force inline" expression or defined away.
// You generally don't need to use forced inlining with the Microsoft and
// Metrowerks compilers, but you may need it with the GCC compiler (any version).
//
// Example usage:
//     template <typename T, typename Allocator>
//     FERMAT_FORCE_INLINE typename vector<T, Allocator>::size_type
//     vector<T, Allocator>::size() const
//        { return mpEnd - mpBegin; }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_FORCE_INLINE
#define FERMAT_FORCE_INLINE KM_FORCE_INLINE
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_MAY_ALIAS
//
// Defined as a macro that wraps the GCC may_alias attribute. This attribute
// has no significance for VC++ because VC++ doesn't support the concept of
// strict aliasing. Users should avoid writing code that breaks strict
// aliasing rules; FERMAT_MAY_ALIAS is for cases with no alternative.
//
// Example usage:
//    uint32_t value FERMAT_MAY_ALIAS;
//
// Example usage:
//    typedef uint32_t FERMAT_MAY_ALIAS value_type;
//    value_type value;
//
#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 303) && !defined(KM_COMPILER_RVCT)
#define FERMAT_MAY_ALIAS __attribute__((__may_alias__))
#else
#define FERMAT_MAY_ALIAS
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_LIKELY / FERMAT_UNLIKELY
//
// Defined as a macro which gives a hint to the compiler for branch
// prediction. GCC gives you the ability to manually give a hint to
// the compiler about the result of a comparison, though it's often
// best to compile shipping code with profiling feedback under both
// GCC (-fprofile-arcs) and VC++ (/LTCG:PGO, etc.). However, there
// are times when you feel very sure that a boolean expression will
// usually evaluate to either true or false and can help the compiler
// by using an explicity directive...
//
// Example usage:
//    if(FERMAT_LIKELY(a == 0)) // Tell the compiler that a will usually equal 0.
//       { ... }
//
// Example usage:
//    if(FERMAT_UNLIKELY(a == 0)) // Tell the compiler that a will usually not equal 0.
//       { ... }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_LIKELY
#if defined(__GNUC__) && (__GNUC__ >= 3)
#define FERMAT_LIKELY(x)   __builtin_expect(!!(x), true)
#define FERMAT_UNLIKELY(x) __builtin_expect(!!(x), false)
#else
#define FERMAT_LIKELY(x)   (x)
#define FERMAT_UNLIKELY(x) (x)
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_NOMINMAX
//
// Defined as 0 or 1; default is 1.
// MSVC++ has #defines for min/max which collide with the min/max algorithm
// declarations. If FERMAT_NOMINMAX is defined as 1, then we undefine min and
// max if they are #defined by an external library. This allows our min and
// max definitions in algorithm.h to work as expected. An alternative to
// the enabling of FERMAT_NOMINMAX is to #define NOMINMAX in your project
// settings if you are compiling for Windows.
// Note that this does not control the availability of the EASTL min and max
// algorithms; the FERMAT_MINMAX_ENABLED configuration parameter does that.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_NOMINMAX
#define FERMAT_NOMINMAX 1
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_STD_CPP_ONLY
//
// Defined as 0 or 1; default is 0.
// Disables the use of compiler language extensions. We use compiler language
// extensions only in the case that they provide some benefit that can't be
// had any other practical way. But sometimes the compiler is set to disable
// language extensions or sometimes one compiler's preprocesor is used to generate
// code for another compiler, and so it's necessary to disable language extension usage.
//
// Example usage:
//     #if defined(_MSC_VER) && !FERMAT_STD_CPP_ONLY
//         enum : size_type { npos = container_type::npos };    // Microsoft extension which results in significantly smaller debug symbols.
//     #else
//         static const size_type npos = container_type::npos;
//     #endif
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_STD_CPP_ONLY
#define FERMAT_STD_CPP_ONLY 0
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_VARIABLE_TEMPLATES_ENABLED
//
// Defined as 0 or 1.
// If enabled then C++11-like functionality with variable templates is enabled.
///////////////////////////////////////////////////////////////////////////////
#if !defined(FERMAT_VARIABLE_TEMPLATES_ENABLED)
#if((EABASE_VERSION_N < 20605) || defined(KM_COMPILER_NO_VARIABLE_TEMPLATES))
#define FERMAT_VARIABLE_TEMPLATES_ENABLED 0
#else
#define FERMAT_VARIABLE_TEMPLATES_ENABLED 1
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// FERMAT_INLINE_VARIABLE_ENABLED
//
// Defined as 0 or 1.
// If enabled then C++17-like functionality with inline variable is enabled.
///////////////////////////////////////////////////////////////////////////////
#if !defined(FERMAT_INLINE_VARIABLE_ENABLED)
#if((EABASE_VERSION_N < 20707) || defined(KM_COMPILER_NO_INLINE_VARIABLES))
#define FERMAT_INLINE_VARIABLE_ENABLED 0
#else
#define FERMAT_INLINE_VARIABLE_ENABLED 1
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// FERMAT_CPP17_INLINE_VARIABLE
//
// Used to prefix a variable as inline when C++17 inline variables are available
// Usage: FERMAT_CPP17_INLINE_VARIABLE constexpr bool type_trait_v = type_trait::value
///////////////////////////////////////////////////////////////////////////////
#if !defined(FERMAT_CPP17_INLINE_VARIABLE)
#if FERMAT_INLINE_VARIABLE_ENABLED
#define FERMAT_CPP17_INLINE_VARIABLE inline
#else
#define FERMAT_CPP17_INLINE_VARIABLE
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// KM_COMPILER_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS undef
//
// We need revise this macro to be undefined in some cases, in case the user
// isn't using an updated EABase.
///////////////////////////////////////////////////////////////////////////////
#if defined(__EDG_VERSION__) && (__EDG_VERSION__ >= 403) // It may in fact be supported by 4.01 or 4.02 but we don't have compilers to test with.

#if defined(KM_COMPILER_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS)
#undef KM_COMPILER_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_ALIGN_OF
//
// Determines the alignment of a type.
//
// Example usage:
//    size_t alignment = FERMAT_ALIGN_OF(int);
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FERMAT_ALIGN_OF
#define FERMAT_ALIGN_OF alignof
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_ALLOCATOR_EXPLICIT_ENABLED
//
// Defined as 0 or 1. Default is 0 for now but ideally would be changed to
// 1 some day. It's 0 because setting it to 1 breaks some existing code.
// This option enables the allocator ctor to be explicit, which avoids
// some undesirable silent conversions, especially with the string class.
//
// Example usage:
//     class allocator
//     {
//     public:
//         FERMAT_ALLOCATOR_EXPLICIT allocator(const char* pName);
//     };
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_ALLOCATOR_EXPLICIT_ENABLED
#define FERMAT_ALLOCATOR_EXPLICIT_ENABLED 0
#endif

#if FERMAT_ALLOCATOR_EXPLICIT_ENABLED
#define FERMAT_ALLOCATOR_EXPLICIT explicit
#else
#define FERMAT_ALLOCATOR_EXPLICIT
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_ALLOCATOR_MIN_ALIGNMENT
//
// Defined as an integral power-of-2 that's >= 1.
// Identifies the minimum alignment that EASTL should assume its allocators
// use. There is code within EASTL that decides whether to do a Malloc or
// MallocAligned call and it's typically better if it can use the Malloc call.
// But this requires knowing what the minimum possible alignment is.
#if !defined(FERMAT_ALLOCATOR_MIN_ALIGNMENT)
#define FERMAT_ALLOCATOR_MIN_ALIGNMENT KM_PLATFORM_MIN_MALLOC_ALIGNMENT
#endif


///////////////////////////////////////////////////////////////////////////////
// FERMAT_SYSTEM_ALLOCATOR_MIN_ALIGNMENT
//
// Identifies the minimum alignment that EASTL should assume system allocations
// from malloc and new will have.
#if !defined(FERMAT_SYSTEM_ALLOCATOR_MIN_ALIGNMENT)
#if defined(KM_PLATFORM_MICROSOFT) || defined(KM_PLATFORM_APPLE)
#define FERMAT_SYSTEM_ALLOCATOR_MIN_ALIGNMENT 16
#else
#define FERMAT_SYSTEM_ALLOCATOR_MIN_ALIGNMENT (KM_PLATFORM_PTR_SIZE * 2)
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EASTL allocator
//
// The EASTL allocator system allows you to redefine how memory is allocated
// via some defines that are set up here. In the container code, memory is
// allocated via macros which expand to whatever the user has them set to
// expand to. Given that there are multiple allocator systems available,
// this system allows you to configure it to use whatever system you want,
// provided your system meets the requirements of this library.
// The requirements are:
//
//     - Must be constructable via a const char* (name) parameter.
//       Some uses of allocators won't require this, however.
//     - Allocate a block of memory of size n and debug name string.
//     - Allocate a block of memory of size n, debug name string,
//       alignment a, and offset o.
//     - Free memory allocated via either of the allocation functions above.
//     - Provide a default allocator instance which can be used if the user
//       doesn't provide a specific one.
//
///////////////////////////////////////////////////////////////////////////////

// namespace fermat
// {
//     class allocator
//     {
//         allocator(const char* pName = NULL);
//
//         void* allocate(size_t n, int flags = 0);
//         void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
//         void  deallocate(void* p, size_t n);
//
//		   // optional:
//		   template<typename T, typename... Args>
//		   void construct(T* p, Args&&... args);
//
//         const char* get_name() const;
//         void        set_name(const char* pName);
//     };
//
//     allocator* GetDefaultAllocator(); // This is used for anonymous allocations.
// }

#ifndef EASTLAlloc // To consider: Instead of calling through pAllocator, just go directly to operator new, since that's what allocator does.

#define EASTLAlloc(allocator, n) (allocator).allocate(n);
#endif

#ifndef EASTLAllocFlags // To consider: Instead of calling through pAllocator, just go directly to operator new, since that's what allocator does.

#define EASTLAllocFlags(allocator, n, flags) (allocator).allocate(n, flags);
#endif

#ifndef EASTLAllocAligned
#define EASTLAllocAligned(allocator, n, alignment, offset) (allocator).allocate((n), (alignment), (offset))
#endif

#ifndef EASTLAllocAlignedFlags
#define EASTLAllocAlignedFlags(allocator, n, alignment, offset, flags) (allocator).allocate((n), (alignment), (offset), (flags))
#endif

#ifndef EASTLFree
#define EASTLFree(allocator, p, size) (allocator).deallocate((void*)(p), (size)) // Important to cast to void* as p may be non-const.

#endif


#ifndef EASTLAllocatorDefault
// EASTLAllocatorDefault returns the default allocator instance. This is not a global
// allocator which implements all container allocations but is the allocator that is
// used when EASTL needs to allocate memory internally. There are very few cases where
// EASTL allocates memory internally, and in each of these it is for a sensible reason
// that is documented to behave as such.
#define EASTLAllocatorDefault fermat::GetDefaultAllocator
#endif


/// FERMAT_ALLOCATOR_DEFAULT_NAME
///
/// Defines a default allocator name in the absence of a user-provided name.
///
#ifndef FERMAT_ALLOCATOR_DEFAULT_NAME
#define FERMAT_ALLOCATOR_DEFAULT_NAME FERMAT_DEFAULT_NAME_PREFIX // Unless the user overrides something, this is "EASTL".

#endif

/// FERMAT_USE_FORWARD_WORKAROUND
///
/// This is to workaround a compiler bug that we found in VS2013. Update 1 did not fix it.
/// This should be fixed in a future release of VS2013 http://accentuable4.rssing.com/browser.php?indx=3511740&item=15696
///
#ifndef FERMAT_USE_FORWARD_WORKAROUND
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER == 180021005 || (defined(__EDG_VERSION__) && (__EDG_VERSION__ < 405))// VS2013 initial release

#define FERMAT_USE_FORWARD_WORKAROUND 1
#else
#define FERMAT_USE_FORWARD_WORKAROUND 0
#endif
#endif

/// FERMAT_OPENSOURCE
/// This is enabled when EASTL is building built in an "open source" mode.  Which is a mode that eliminates code
/// dependencies on other technologies that have not been released publically.
/// FERMAT_OPENSOURCE = 0, is the default.
/// FERMAT_OPENSOURCE = 1, utilizes technologies that not publically available.
///
#ifndef FERMAT_OPENSOURCE
#define FERMAT_OPENSOURCE 0
#endif

/// FERMAT_HAS_INTRINSIC(x)
///   does the compiler intrinsic (MSVC terminology) or builtin (Clang / GCC terminology) exist?
///   where `x` does not include the leading "__"
#if defined(KM_COMPILER_CLANG)
// see https://clang.llvm.org/docs/LanguageExtensions.html#type-trait-primitives
#if KM_COMPILER_VERSION >= 1000
#define FERMAT_HAS_INTRINSIC(x) KM_COMPILER_HAS_BUILTIN(__ ## x)
#elif KM_COMPILER_VERSION >= 600
// NB: !__is_identifier() is correct: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66970#c11
#define FERMAT_HAS_INTRINSIC(x) !__is_identifier(__ ## x)
#else
// note: only works for a subset of builtins
#define FERMAT_HAS_INTRINSIC(x) KM_COMPILER_HAS_FEATURE(x)
#endif
#else
#define FERMAT_HAS_INTRINSIC(x) KM_COMPILER_HAS_BUILTIN(__ ## x)
#endif

/// FERMAT_ENABLE_PAIR_FIRST_ELEMENT_CONSTRUCTOR
/// This feature define allows users to toggle the problematic std::pair implicit
/// single element constructor.
#ifndef FERMAT_ENABLE_PAIR_FIRST_ELEMENT_CONSTRUCTOR
#define FERMAT_ENABLE_PAIR_FIRST_ELEMENT_CONSTRUCTOR 0
#endif

/// FERMAT_SYSTEM_BIG_ENDIAN_STATEMENT
/// FERMAT_SYSTEM_LITTLE_ENDIAN_STATEMENT
/// These macros allow you to write endian specific macros as statements.
/// This allows endian specific code to be macro expanded from within other macros
///
#if defined(KM_SYSTEM_BIG_ENDIAN)
#define FERMAT_SYSTEM_BIG_ENDIAN_STATEMENT(...) __VA_ARGS__
#else
#define FERMAT_SYSTEM_BIG_ENDIAN_STATEMENT(...)
#endif

#if defined(KM_SYSTEM_LITTLE_ENDIAN)
#define FERMAT_SYSTEM_LITTLE_ENDIAN_STATEMENT(...) __VA_ARGS__
#else
#define FERMAT_SYSTEM_LITTLE_ENDIAN_STATEMENT(...)
#endif

/// FERMAT_CONSTEXPR_BIT_CAST_SUPPORTED
/// fermat::bit_cast, in order to be implemented as constexpr, requires explicit compiler support.
/// This macro defines whether it's possible for bit_cast to be constexpr.
///
#if (defined(KM_COMPILER_MSVC) && defined(KM_COMPILER_MSVC_VERSION_14_26) && KM_COMPILER_VERSION >= KM_COMPILER_MSVC_VERSION_14_26) \
	|| KM_COMPILER_HAS_BUILTIN(__builtin_bit_cast)
#define FERMAT_CONSTEXPR_BIT_CAST_SUPPORTED 1
#else
#define FERMAT_CONSTEXPR_BIT_CAST_SUPPORTED 0
#endif

// FERMAT_DEPRECATIONS_FOR_2024_SEPT
// This macro is provided as a means to disable warnings temporarily (in particular if a user is compiling with warnings
// as errors). All deprecations raised by this macro (when it is KM_ENABLED) are scheduled for removal approximately
// September 2024.

#ifndef FERMAT_DEPRECATIONS_FOR_2024_SEPT
#if defined(KM_DEPRECATIONS_FOR_2024_SEPT)
#define FERMAT_DEPRECATIONS_FOR_2024_SEPT KM_DEPRECATIONS_FOR_2024_SEPT
#else
#define FERMAT_DEPRECATIONS_FOR_2024_SEPT KM_ENABLED
#endif
#endif

#if KM_IS_ENABLED(FERMAT_DEPRECATIONS_FOR_2024_SEPT)
#define FERMAT_REMOVE_AT_2024_SEPT KM_DEPRECATED
#else
#define FERMAT_REMOVE_AT_2024_SEPT
#endif

// For internal (to EASTL) use only (ie. tests).
#define FERMAT_INTERNAL_DISABLE_DEPRECATED()					\
	KM_DISABLE_VC_WARNING(4996);							\
	KM_DISABLE_CLANG_WARNING(-Wdeprecated-declarations);	\
	KM_DISABLE_GCC_WARNING(-Wdeprecated-declarations);

// For internal (to EASTL) use only (ie. tests).
#define FERMAT_INTERNAL_RESTORE_DEPRECATED()	\
	KM_RESTORE_CLANG_WARNING();				\
	KM_RESTORE_VC_WARNING();				\
	KM_RESTORE_GCC_WARNING();

// FERMAT_ATOMIC_READ_DEPENDS_IS_ACQUIRE, this determines if we want the semantics of EASTL's
// read_depends memory order to be strengthened to acquire, the default is for read_depends to
// do relaxed semantics since it's intended for performance sensitive code with very specific
// semantics, see atomic.h for details.
#ifndef FERMAT_ATOMIC_READ_DEPENDS_IS_ACQUIRE
#define FERMAT_ATOMIC_READ_DEPENDS_IS_ACQUIRE KM_DISABLED
#endif

// FERMAT_INTERNAL_TSAN_ACQUIRE for EASTL internal use only. Instrument an acquire operation
// on an addres for TSAN purposes
#if KM_TSAN_ENABLED
#define FERMAT_INTERNAL_TSAN_ACQUIRE(x) __tsan_acquire(x)
#else
#define FERMAT_INTERNAL_TSAN_ACQUIRE(x)
#endif
