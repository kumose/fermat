/*-----------------------------------------------------------------------------
 * config/eacompilertraits.h
 *
 * Copyright (c) Electronic Arts Inc. All rights reserved.
 *-----------------------------------------------------------------------------
 * Currently supported defines include:
 *    KM_PREPROCESSOR_JOIN
 *    
 *    KM_COMPILER_IS_ANSIC
 *    KM_COMPILER_IS_C99
 *    KM_COMPILER_IS_C11
 *    KM_COMPILER_HAS_C99_TYPES
 *    KM_COMPILER_IS_CPLUSPLUS
 *    KM_COMPILER_MANAGED_CPP
 *    KM_COMPILER_INTMAX_SIZE
 *    KM_OFFSETOF
 *    KM_SIZEOF_MEMBER
 *
 *    KM_ALIGN_OF()
 *    KM_ALIGN_MAX_STATIC / KM_ALIGN_MAX_AUTOMATIC
 *    KM_ALIGN() / KM_PREFIX_ALIGN() / KM_POSTFIX_ALIGN()
 *    KM_ALIGNED()
 *    KM_PACKED()
 *
 *    KM_LIKELY()
 *    KM_UNLIKELY()
 *    KM_INIT_PRIORITY()
 *    KM_MAY_ALIAS()
 *    KM_ASSUME()
 *    KM_ANALYSIS_ASSUME()
 *    KM_PURE
 *    KM_WEAK
 *    KM_UNUSED()
 *    KM_EMPTY()
 *
 *    KM_WCHAR_T_NON_NATIVE
 *    KM_WCHAR_SIZE = <n bytes>
 *
 *    KM_RESTRICT
 *    KM_DEPRECATED   / KM_PREFIX_DEPRECATED   / KM_POSTFIX_DEPRECATED
 *    KM_FORCE_INLINE / KM_PREFIX_FORCE_INLINE / KM_POSTFIX_FORCE_INLINE
 *    KM_NO_INLINE    / KM_PREFIX_NO_INLINE    / KM_POSTFIX_NO_INLINE
 *    KM_NO_VTABLE    / KM_CLASS_NO_VTABLE     / KM_STRUCT_NO_VTABLE
 *    KM_PASCAL
 *    KM_PASCAL_FUNC()
 *    KM_SSE = [0 | 1]
 *    KM_IMPORT
 *    KM_EXPORT
 *    KM_PRAGMA_ONCE_SUPPORTED
 *    KM_ONCE
 *    KM_OVERRIDE
 *    KM_INHERITANCE_FINAL
 *    KM_SEALED
 *    KM_ABSTRACT
 *    constexpr / constexpr
 *    if constexpr
 *    KM_EXTERN_TEMPLATE
 *    KM_NORETURN
 *    KM_CARRIES_DEPENDENCY
 *    KM_NON_COPYABLE / struct EANonCopyable
 *    KM_OPTIMIZE_OFF / KM_OPTIMIZE_ON
 *    KM_SIGNED_RIGHT_SHIFT_IS_UNSIGNED
 *
 *    KM_DISABLE_VC_WARNING    / KM_RESTORE_VC_WARNING / KM_DISABLE_ALL_VC_WARNINGS / KM_RESTORE_ALL_VC_WARNINGS
 *    KM_DISABLE_GCC_WARNING   / KM_RESTORE_GCC_WARNING
 *    KM_DISABLE_CLANG_WARNING / KM_RESTORE_CLANG_WARNING
 *    KM_DISABLE_SN_WARNING    / KM_RESTORE_SN_WARNING / KM_DISABLE_ALL_SN_WARNINGS / KM_RESTORE_ALL_SN_WARNINGS
 *    KM_DISABLE_GHS_WARNING   / KM_RESTORE_GHS_WARNING
 *    KM_DISABLE_EDG_WARNING   / KM_RESTORE_EDG_WARNING
 *    KM_DISABLE_CW_WARNING    / KM_RESTORE_CW_WARNING
 *
 *    KM_DISABLE_DEFAULT_CTOR
 *    KM_DISABLE_COPY_CTOR
 *    KM_DISABLE_MOVE_CTOR
 *    KM_DISABLE_ASSIGNMENT_OPERATOR
 *    KM_DISABLE_MOVE_OPERATOR
 *
 *  Todo:
 *    Find a way to reliably detect wchar_t size at preprocessor time and 
 *    implement it below for KM_WCHAR_SIZE.
 *
 *  Todo:
 *    Find out how to support KM_PASCAL and KM_PASCAL_FUNC for systems in
 *    which it hasn't yet been found out for.
 *---------------------------------------------------------------------------*/


#ifndef INCLUDED_eacompilertraits_H
#define INCLUDED_eacompilertraits_H

	#include <fermat/base/config/eaplatform.h>
	#include <fermat/base/config/eacompiler.h>


	// Metrowerks uses #defines in its core C header files to define 
	// the kind of information we need below (e.g. C99 compatibility)



	// Determine if this compiler is ANSI C compliant and if it is C99 compliant.
	#if defined(__STDC__)
		#define KM_COMPILER_IS_ANSIC 1    // The compiler claims to be ANSI C

		// Is the compiler a C99 compiler or equivalent?
		// From ISO/IEC 9899:1999:
		//    6.10.8 Predefined macro names
		//    __STDC_VERSION__ The integer constant 199901L. (150)
		//
		//    150) This macro was not specified in ISO/IEC 9899:1990 and was 
		//    specified as 199409L in ISO/IEC 9899/AMD1:1995. The intention 
		//    is that this will remain an integer constant of type long int 
		//    that is increased with each revision of this International Standard.
		//
		#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
			#define KM_COMPILER_IS_C99 1
		#endif

 		// Is the compiler a C11 compiler?
 		// From ISO/IEC 9899:2011:
		//   Page 176, 6.10.8.1 (Predefined macro names) :
 		//   __STDC_VERSION__ The integer constant 201112L. (178)
		//
		#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
			#define KM_COMPILER_IS_C11 1
		#endif
	#endif

	// Some compilers (e.g. GCC) define __USE_ISOC99 if they are not 
	// strictly C99 compilers (or are simply C++ compilers) but are set 
	// to use C99 functionality. Metrowerks defines _MSL_C99 as 1 in 
	// this case, but 0 otherwise.
	#if (defined(__USE_ISOC99) || (defined(_MSL_C99) && (_MSL_C99 == 1))) && !defined(KM_COMPILER_IS_C99)
		#define KM_COMPILER_IS_C99 1
	#endif
 
	// Metrowerks defines C99 types (e.g. intptr_t) instrinsically when in C99 mode (-lang C99 on the command line).
	#if (defined(_MSL_C99) && (_MSL_C99 == 1))
		#define KM_COMPILER_HAS_C99_TYPES 1
	#endif

	#if defined(__GNUC__) 
		#if (((__GNUC__ * 100) + __GNUC_MINOR__) >= 302) // Also, GCC defines _HAS_C9X.
			#define KM_COMPILER_HAS_C99_TYPES 1 // The compiler is not necessarily a C99 compiler, but it defines C99 types.
			
			#ifndef __STDC_LIMIT_MACROS
				#define __STDC_LIMIT_MACROS 1
			#endif
			
			#ifndef __STDC_CONSTANT_MACROS
				#define __STDC_CONSTANT_MACROS 1    // This tells the GCC compiler that we want it to use its native C99 types.
			#endif
		#endif
	#endif

	#if defined(_MSC_VER) && (_MSC_VER >= 1600)
		#define KM_COMPILER_HAS_C99_TYPES 1
	#endif

	#ifdef  __cplusplus
		#define KM_COMPILER_IS_CPLUSPLUS 1
	#endif


	// ------------------------------------------------------------------------
	// KM_PREPROCESSOR_JOIN
	//
	// This macro joins the two arguments together, even when one of  
	// the arguments is itself a macro (see 16.3.1 in C++98 standard). 
	// This is often used to create a unique name with __LINE__.
	//
	// For example, this declaration:
	//    char KM_PREPROCESSOR_JOIN(unique_, __LINE__);
	// expands to this:
	//    char unique_73;
	//
	// Note that all versions of MSVC++ up to at least version 7.1 
	// fail to properly compile macros that use __LINE__ in them
	// when the "program database for edit and continue" option
	// is enabled. The result is that __LINE__ gets converted to 
	// something like __LINE__(Var+37).
	//
	#ifndef KM_PREPROCESSOR_JOIN
		#define KM_PREPROCESSOR_JOIN(a, b)  KM_PREPROCESSOR_JOIN1(a, b)
		#define KM_PREPROCESSOR_JOIN1(a, b) KM_PREPROCESSOR_JOIN2(a, b)
		#define KM_PREPROCESSOR_JOIN2(a, b) a##b
	#endif


	// ------------------------------------------------------------------------
	// KM_STRINGIFY
	//
	// Example usage:
	//     printf("Line: %s", KM_STRINGIFY(__LINE__));
	//
	#ifndef KM_STRINGIFY
		#define KM_STRINGIFY(x)     KM_STRINGIFYIMPL(x)
		#define KM_STRINGIFYIMPL(x) #x
	#endif


	// ------------------------------------------------------------------------
	// KM_IDENTITY
	//
	#ifndef KM_IDENTITY
		#define KM_IDENTITY(x) x
	#endif


	// ------------------------------------------------------------------------
	// KM_COMPILER_MANAGED_CPP
	// Defined if this is being compiled with Managed C++ extensions
	#ifdef KM_COMPILER_MSVC
		#if KM_COMPILER_VERSION >= 1300
			#ifdef _MANAGED
				#define KM_COMPILER_MANAGED_CPP 1
			#endif
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_COMPILER_INTMAX_SIZE
	//
	// This is related to the concept of intmax_t uintmax_t, but is available 
	// in preprocessor form as opposed to compile-time form. At compile-time
	// you can use intmax_t and uintmax_t to use the actual types.
	//
	#if defined(__GNUC__) && defined(__x86_64__)
		#define KM_COMPILER_INTMAX_SIZE 16  // intmax_t is __int128_t (GCC extension) and is 16 bytes.
	#else
		#define KM_COMPILER_INTMAX_SIZE 8   // intmax_t is int64_t and is 8 bytes.
	#endif



	// ------------------------------------------------------------------------
	// KM_LPAREN / KM_RPAREN / KM_COMMA / KM_SEMI
	//
	// These are used for using special characters in macro-using expressions.
	// Note that this macro intentionally uses (), as in some cases it can't 
	// work unless it does.
	//
	// Example usage:
	//     int x = SOME_MACRO(SomeTemplate<int KM_COMMA() int EACOMMA() char>);
	//
	#ifndef KM_LPAREN
		#define KM_LPAREN() (
	#endif
	#ifndef KM_RPAREN
		#define KM_RPAREN() )
	#endif
	#ifndef KM_COMMA
		#define KM_COMMA()  ,
	#endif
	#ifndef KM_SEMI
		#define KM_SEMI()   ;
	#endif




	// ------------------------------------------------------------------------
	// KM_OFFSETOF
	// Implements a portable version of the non-standard offsetof macro.
	//
	// The offsetof macro is guaranteed to only work with POD types. However, we wish to use
	// it for non-POD types but where we know that offsetof will still work for the cases 
	// in which we use it. GCC unilaterally gives a warning when using offsetof with a non-POD,
	// even if the given usage happens to work. So we make a workaround version of offsetof
	// here for GCC which has the same effect but tricks the compiler into not issuing the warning.
	// The 65536 does the compiler fooling; the reinterpret_cast prevents the possibility of
	// an overloaded operator& for the class getting in the way.
	//
	// Example usage:
	//     struct A{ int x; int y; };
	//     size_t n = KM_OFFSETOF(A, y);
	//
	#if defined(__GNUC__)                       // We can't use GCC 4's __builtin_offsetof because it mistakenly complains about non-PODs that are really PODs.
		#define KM_OFFSETOF(struct_, member_)  ((size_t)(((uintptr_t)&reinterpret_cast<const volatile char&>((((struct_*)65536)->member_))) - 65536))
	#else
		#define KM_OFFSETOF(struct_, member_)  offsetof(struct_, member_)
	#endif

	// ------------------------------------------------------------------------
	// KM_SIZEOF_MEMBER
	// Implements a portable way to determine the size of a member. 
	//
	// The KM_SIZEOF_MEMBER simply returns the size of a member within a class or struct; member
	// access rules still apply. We offer two approaches depending on the compiler's support for non-static member 
	// initializers although most C++11 compilers support this.
	//
	// Example usage:
	//     struct A{ int x; int y; };
	//     size_t n = KM_SIZEOF_MEMBER(A, y);
	//
	#ifndef KM_COMPILER_NO_EXTENDED_SIZEOF
		#define KM_SIZEOF_MEMBER(struct_, member_) (sizeof(struct_::member_))
	#else
		#define KM_SIZEOF_MEMBER(struct_, member_) (sizeof(((struct_*)0)->member_))
	#endif

	// ------------------------------------------------------------------------
	// alignment expressions
	//
	// Here we define
	//    KM_ALIGN_OF(type)         // Returns size_t.
	//    KM_ALIGN_MAX_STATIC       // The max align value that the compiler will respect for KM_ALIGN for static data (global and static variables). Some compilers allow high values, some allow no more than 8. KM_ALIGN_MIN is assumed to be 1.
	//    KM_ALIGN_MAX_AUTOMATIC    // The max align value for automatic variables (variables declared as local to a function).
	//    KM_ALIGN(n)               // Used as a prefix. n is byte alignment, with being a power of two. Most of the time you can use this and avoid using KM_PREFIX_ALIGN/KM_POSTFIX_ALIGN.
	//    KM_ALIGNED(t, v, n)       // Type, variable, alignment. Used to align an instance. You should need this only for unusual compilers.
	//    KM_PACKED                 // Specifies that the given structure be packed (and not have its members aligned).
	//
	// Also we define the following for rare cases that it's needed.
	//    KM_PREFIX_ALIGN(n)        // n is byte alignment, with being a power of two. You should need this only for unusual compilers.
	//    KM_POSTFIX_ALIGN(n)       // Valid values for n are 1, 2, 4, 8, etc. You should need this only for unusual compilers.
	//
	// Example usage:
	//    size_t x = KM_ALIGN_OF(int);                                  Non-aligned equivalents.        Meaning
	//    KM_PREFIX_ALIGN(8) int x = 5;                                 int x = 5;                      Align x on 8 for compilers that require prefix attributes. Can just use KM_ALIGN instead.
	//    KM_ALIGN(8) int x;                                            int x;                          Align x on 8 for compilers that allow prefix attributes.
	//    int x KM_POSTFIX_ALIGN(8);                                    int x;                          Align x on 8 for compilers that require postfix attributes.
	//    int x KM_POSTFIX_ALIGN(8) = 5;                                int x = 5;                      Align x on 8 for compilers that require postfix attributes.
	//    int x KM_POSTFIX_ALIGN(8)(5);                                 int x(5);                       Align x on 8 for compilers that require postfix attributes.
	//    struct KM_PREFIX_ALIGN(8) X { int x; } KM_POSTFIX_ALIGN(8);   struct X { int x; };            Define X as a struct which is aligned on 8 when used.
	//    KM_ALIGNED(int, x, 8) = 5;                                    int x = 5;                      Align x on 8.
	//    KM_ALIGNED(int, x, 16)(5);                                    int x(5);                       Align x on 16.
	//    KM_ALIGNED(int, x[3], 16);                                    int x[3];                       Align x array on 16.
	//    KM_ALIGNED(int, x[3], 16) = { 1, 2, 3 };                      int x[3] = { 1, 2, 3 };         Align x array on 16.
	//    int x[3] KM_PACKED;                                           int x[3];                       Pack the 3 ints of the x array. GCC doesn't seem to support packing of int arrays.
	//    struct KM_ALIGN(32) X { int x; int y; };                      struct X { int x; };            Define A as a struct which is aligned on 32 when used.
	//    KM_ALIGN(32) struct X { int x; int y; } Z;                    struct X { int x; } Z;          Define A as a struct, and align the instance Z on 32.
	//    struct X { int x KM_PACKED; int y KM_PACKED; };               struct X { int x; int y; };     Pack the x and y members of struct X.
	//    struct X { int x; int y; } KM_PACKED;                         struct X { int x; int y; };     Pack the members of struct X.
	//    typedef KM_ALIGNED(int, int16, 16); int16 n16;                typedef int int16; int16 n16;   Define int16 as an int which is aligned on 16.
	//    typedef KM_ALIGNED(X, X16, 16); X16 x16;                      typedef X X16; X16 x16;         Define X16 as an X which is aligned on 16.

	#if !defined(KM_ALIGN_MAX)                              // If the user hasn't globally set an alternative value...
		#if defined(KM_PROCESSOR_ARM)                       // ARM compilers in general tend to limit automatic variables to 8 or less.
			#define KM_ALIGN_MAX_STATIC    1048576
			#define KM_ALIGN_MAX_AUTOMATIC       1          // Typically they support only built-in natural aligment types (both arm-eabi and apple-abi).
		#elif defined(KM_PLATFORM_APPLE)
			#define KM_ALIGN_MAX_STATIC    1048576
			#define KM_ALIGN_MAX_AUTOMATIC      16
		#else
			#define KM_ALIGN_MAX_STATIC    1048576          // Arbitrarily high value. What is the actual max?
			#define KM_ALIGN_MAX_AUTOMATIC 1048576
		#endif
	#endif

	// EDG intends to be compatible with GCC but has a bug whereby it 
	// fails to support calling a constructor in an aligned declaration when 
	// using postfix alignment attributes. Prefix works for alignment, but does not align
	// the size like postfix does.  Prefix also fails on templates.  So gcc style post fix
	// is still used, but the user will need to use KM_POSTFIX_ALIGN before the constructor parameters.
	#if defined(__GNUC__) && (__GNUC__ < 3)
		#define KM_ALIGN_OF(type) ((size_t)__alignof__(type))
		#define KM_ALIGN(n)
		#define KM_PREFIX_ALIGN(n)
		#define KM_POSTFIX_ALIGN(n) __attribute__((aligned(n)))
		#define KM_ALIGNED(variable_type, variable, n) variable_type variable __attribute__((aligned(n)))
		#define KM_PACKED __attribute__((packed))

	// GCC 3.x+, IBM, and clang support prefix attributes.
	#elif (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__xlC__) || defined(__clang__)
		#define KM_ALIGN_OF(type) ((size_t)__alignof__(type))
		#define KM_ALIGN(n) __attribute__((aligned(n)))
		#define KM_PREFIX_ALIGN(n)
		#define KM_POSTFIX_ALIGN(n) __attribute__((aligned(n)))
		#define KM_ALIGNED(variable_type, variable, n) variable_type variable __attribute__((aligned(n)))
		#define KM_PACKED __attribute__((packed))

	// Metrowerks supports prefix attributes.
	// Metrowerks does not support packed alignment attributes.
	#elif defined(KM_COMPILER_INTEL) || defined(CS_UNDEFINED_STRING) || (defined(KM_COMPILER_MSVC) && (KM_COMPILER_VERSION >= 1300))
		#define KM_ALIGN_OF(type) ((size_t)__alignof(type))
		#define KM_ALIGN(n) __declspec(align(n))
		#define KM_PREFIX_ALIGN(n) KM_ALIGN(n)
		#define KM_POSTFIX_ALIGN(n)
		#define KM_ALIGNED(variable_type, variable, n) KM_ALIGN(n) variable_type variable
		#define KM_PACKED // See KM_PRAGMA_PACK_VC for an alternative.

	// Arm brand compiler
	#elif defined(KM_COMPILER_ARM)
		#define KM_ALIGN_OF(type) ((size_t)__ALIGNOF__(type))
		#define KM_ALIGN(n) __align(n)
		#define KM_PREFIX_ALIGN(n) __align(n)
		#define KM_POSTFIX_ALIGN(n)
		#define KM_ALIGNED(variable_type, variable, n) __align(n) variable_type variable
		#define KM_PACKED __packed

	#else // Unusual compilers
		// There is nothing we can do about some of these. This is not as bad a problem as it seems.
		// If the given platform/compiler doesn't support alignment specifications, then it's somewhat
		// likely that alignment doesn't matter for that platform. Otherwise they would have defined 
		// functionality to manipulate alignment.
		#define KM_ALIGN(n)
		#define KM_PREFIX_ALIGN(n)
		#define KM_POSTFIX_ALIGN(n)
		#define KM_ALIGNED(variable_type, variable, n) variable_type variable
		#define KM_PACKED

		#ifdef __cplusplus
			template <typename T> struct EAAlignOf1 { enum { s = sizeof (T), value = s ^ (s & (s - 1)) }; };
			template <typename T> struct EAAlignOf2;
			template <int size_diff> struct helper { template <typename T> struct Val { enum { value = size_diff }; }; };
			template <> struct helper<0> { template <typename T> struct Val { enum { value = EAAlignOf2<T>::value }; }; };
			template <typename T> struct EAAlignOf2 { struct Big { T x; char c; };
			enum { diff = sizeof (Big) - sizeof (T), value = helper<diff>::template Val<Big>::value }; };
			template <typename T> struct EAAlignof3 { enum { x = EAAlignOf2<T>::value, y = EAAlignOf1<T>::value, value = x < y ? x : y }; };
			#define KM_ALIGN_OF(type) ((size_t)EAAlignof3<type>::value)

		#else
			// C implementation of KM_ALIGN_OF
			// This implementation works for most cases, but doesn't directly work 
			// for types such as function pointer declarations. To work with those
			// types you need to typedef the type and then use the typedef in KM_ALIGN_OF.
			#define KM_ALIGN_OF(type) ((size_t)offsetof(struct { char c; type m; }, m))
		#endif
	#endif

	// KM_PRAGMA_PACK_VC
	//
	// Wraps #pragma pack in a way that allows for cleaner code.
	// 
	// Example usage:
	//    KM_PRAGMA_PACK_VC(push, 1)
	//    struct X{ char c; int i; };
	//    KM_PRAGMA_PACK_VC(pop)
	//
	#if !defined(KM_PRAGMA_PACK_VC)
		#if defined(KM_COMPILER_MSVC)
			#define KM_PRAGMA_PACK_VC(...) __pragma(pack(__VA_ARGS__))
		#elif !defined(KM_COMPILER_NO_VARIADIC_MACROS)
			#define KM_PRAGMA_PACK_VC(...)
		#else
			// No support. However, all compilers of significance to us support variadic macros.
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_LIKELY / KM_UNLIKELY
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
	//    if(KM_LIKELY(a == 0)) // Tell the compiler that a will usually equal 0.
	//       { ... }
	//
	// Example usage:
	//    if(KM_UNLIKELY(a == 0)) // Tell the compiler that a will usually not equal 0.
	//       { ... }
	//
	#ifndef KM_LIKELY
		#if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
			#if defined(__cplusplus)
				#define KM_LIKELY(x)   __builtin_expect(!!(x), true)
				#define KM_UNLIKELY(x) __builtin_expect(!!(x), false)
			#else
				#define KM_LIKELY(x)   __builtin_expect(!!(x), 1)
				#define KM_UNLIKELY(x) __builtin_expect(!!(x), 0)
			#endif
		#else
			#define KM_LIKELY(x)   (x)
			#define KM_UNLIKELY(x) (x)
		#endif
	#endif

	// ------------------------------------------------------------------------
	// KM_HAS_INCLUDE_AVAILABLE
	//
	// Used to guard against the KM_HAS_INCLUDE() macro on compilers that do not
	// support said feature.
	//
	// Example usage:
	//
	// #if KM_HAS_INCLUDE_AVAILABLE
	//     #if KM_HAS_INCLUDE("myinclude.h")
    //         #include "myinclude.h"
	//     #endif
	// #endif
	#if !defined(KM_HAS_INCLUDE_AVAILABLE)
		#if KM_COMPILER_CPP17_ENABLED || KM_COMPILER_CLANG || KM_COMPILER_GNUC
			#define KM_HAS_INCLUDE_AVAILABLE 1
		#else
			#define KM_HAS_INCLUDE_AVAILABLE 0
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_HAS_INCLUDE
	//
	// May be used in #if and #elif expressions to test for the existence
	// of the header referenced in the operand. If possible it evaluates to a
	// non-zero value and zero otherwise. The operand is the same form as the file
	// in a #include directive.
	//
	// Example usage:
	//
	// #if KM_HAS_INCLUDE("myinclude.h")
	//     #include "myinclude.h"
	// #endif
	//
	// #if KM_HAS_INCLUDE(<myinclude.h>)
	//     #include <myinclude.h>
	// #endif

	#if !defined(KM_HAS_INCLUDE)
		#if KM_COMPILER_CPP17_ENABLED
			#define KM_HAS_INCLUDE(x) __has_include(x)
		#elif KM_COMPILER_CLANG
			#define KM_HAS_INCLUDE(x) __has_include(x)
		#elif KM_COMPILER_GNUC
			#define KM_HAS_INCLUDE(x) __has_include(x)
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_INIT_PRIORITY_AVAILABLE
	//
	// This value is either not defined, or defined to 1.
	// Defines if the GCC attribute init_priority is supported by the compiler.
	//
	#if !defined(KM_INIT_PRIORITY_AVAILABLE)
		#if defined(__GNUC__) && !defined(__EDG__) // EDG typically #defines __GNUC__ but doesn't implement init_priority.
			#define KM_INIT_PRIORITY_AVAILABLE 1
		#elif defined(__clang__)
			#define KM_INIT_PRIORITY_AVAILABLE 1  // Clang implements init_priority
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_INIT_PRIORITY
	//
	// This is simply a wrapper for the GCC init_priority attribute that allows 
	// multiplatform code to be easier to read. This attribute doesn't apply
	// to VC++ because VC++ uses file-level pragmas to control init ordering.
	//
	// Example usage:
	//     SomeClass gSomeClass KM_INIT_PRIORITY(2000);
	//
	#if !defined(KM_INIT_PRIORITY)
		#if defined(KM_INIT_PRIORITY_AVAILABLE)
			#define KM_INIT_PRIORITY(x)  __attribute__ ((init_priority (x)))
		#else
			#define KM_INIT_PRIORITY(x)
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_INIT_SEG_AVAILABLE
	//
	//
	#if !defined(KM_INIT_SEG_AVAILABLE)
		#if defined(_MSC_VER)
			#define KM_INIT_SEG_AVAILABLE 1
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_INIT_SEG
	//
	// Specifies a keyword or code section that affects the order in which startup code is executed.
	//
	// https://docs.microsoft.com/en-us/cpp/preprocessor/init-seg?view=vs-2019
	//
	// Example:
	// 		KM_INIT_SEG(compiler) MyType gMyTypeGlobal;
	// 		KM_INIT_SEG("my_section") MyOtherType gMyOtherTypeGlobal;
	//
	#if !defined(KM_INIT_SEG)
		#if defined(KM_INIT_SEG_AVAILABLE)
			#define KM_INIT_SEG(x)                                                                                                \
				__pragma(warning(push)) __pragma(warning(disable : 4074)) __pragma(warning(disable : 4075)) __pragma(init_seg(x)) \
					__pragma(warning(pop))
		#else
			#define KM_INIT_SEG(x)
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_MAY_ALIAS_AVAILABLE
	//
	// Defined as 0, 1, or 2.
	// Defines if the GCC attribute may_alias is supported by the compiler.
	// Consists of a value 0 (unsupported, shouldn't be used), 1 (some support), 
	// or 2 (full proper support). 
	//
	#ifndef KM_MAY_ALIAS_AVAILABLE
		#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 303)
			#if   !defined(__EDG__)                 // define it as 1 while defining GCC's support as 2.
				#define KM_MAY_ALIAS_AVAILABLE 2
			#else
				#define KM_MAY_ALIAS_AVAILABLE 0
			#endif                                  
		#else 
			#define KM_MAY_ALIAS_AVAILABLE 0
		#endif
	#endif


	// KM_MAY_ALIAS
	//
	// Defined as a macro that wraps the GCC may_alias attribute. This attribute
	// has no significance for VC++ because VC++ doesn't support the concept of 
	// strict aliasing. Users should avoid writing code that breaks strict 
	// aliasing rules; KM_MAY_ALIAS is for cases with no alternative.
	//
	// Example usage:
	//    void* KM_MAY_ALIAS gPtr = NULL;
	//
	// Example usage:
	//    typedef void* KM_MAY_ALIAS pvoid_may_alias;
	//    pvoid_may_alias gPtr = NULL;
	//
	#if KM_MAY_ALIAS_AVAILABLE
		#define KM_MAY_ALIAS __attribute__((__may_alias__))
	#else
		#define KM_MAY_ALIAS
	#endif


	// ------------------------------------------------------------------------
	// KM_ASSUME
	//
	// This acts the same as the VC++ __assume directive and is implemented 
	// simply as a wrapper around it to allow portable usage of it and to take
	// advantage of it if and when it appears in other compilers.
	//
	// Example usage:
	//    void Function(int a) {
	//       switch(a) {
	//          case 1:
	//             DoSomething(1);
	//             break;
	//          case 2:
	//             DoSomething(-1);
	//             break;
	//          default:
	//             KM_ASSUME(0); // This tells the optimizer that the default cannot be reached.
	//       }
	//    }
	//
	#ifndef KM_ASSUME
		#if defined(_MSC_VER) && (_MSC_VER >= 1300) // If VC7.0 and later
			#define KM_ASSUME(x) __assume(x)
		#else
			#define KM_ASSUME(x)
		#endif
	#endif



	// ------------------------------------------------------------------------
	// KM_ANALYSIS_ASSUME
	//
	// This acts the same as the VC++ __analysis_assume directive and is implemented 
	// simply as a wrapper around it to allow portable usage of it and to take
	// advantage of it if and when it appears in other compilers.
	//
	// Example usage:
	//    char Function(char* p) {
	//       KM_ANALYSIS_ASSUME(p != NULL);
	//       return *p;
	//    }
	//
	#ifndef KM_ANALYSIS_ASSUME
		#if defined(_MSC_VER) && (_MSC_VER >= 1300) // If VC7.0 and later 
			#define KM_ANALYSIS_ASSUME(x) __analysis_assume(!!(x)) // !! because that allows for convertible-to-bool in addition to bool.
		#else
			#define KM_ANALYSIS_ASSUME(x)
		#endif
	#endif



	// ------------------------------------------------------------------------
	// KM_DISABLE_VC_WARNING / KM_RESTORE_VC_WARNING
	// 
	// Disable and re-enable warning(s) within code.
	// This is simply a wrapper for VC++ #pragma warning(disable: nnnn) for the
	// purpose of making code easier to read due to avoiding nested compiler ifdefs
	// directly in code.
	//
	// Example usage:
	//     KM_DISABLE_VC_WARNING(4127 3244)
	//     <code>
	//     KM_RESTORE_VC_WARNING()
	//
	#ifndef KM_DISABLE_VC_WARNING
		#if defined(_MSC_VER)
			#define KM_DISABLE_VC_WARNING(w)  \
				__pragma(warning(push))       \
				__pragma(warning(disable:w))
		#else
			#define KM_DISABLE_VC_WARNING(w)
		#endif
	#endif

	#ifndef KM_RESTORE_VC_WARNING
		#if defined(_MSC_VER)
			#define KM_RESTORE_VC_WARNING()   \
				__pragma(warning(pop))
		#else
			#define KM_RESTORE_VC_WARNING()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_ENABLE_VC_WARNING_AS_ERROR / KM_DISABLE_VC_WARNING_AS_ERROR
	//
	// Disable and re-enable treating a warning as error within code.
	// This is simply a wrapper for VC++ #pragma warning(error: nnnn) for the
	// purpose of making code easier to read due to avoiding nested compiler ifdefs
	// directly in code.
	//
	// Example usage:
	//     KM_ENABLE_VC_WARNING_AS_ERROR(4996)
	//     <code>
	//     KM_DISABLE_VC_WARNING_AS_ERROR()
	//
	#ifndef KM_ENABLE_VC_WARNING_AS_ERROR
		#if defined(_MSC_VER)
			#define KM_ENABLE_VC_WARNING_AS_ERROR(w) \
					__pragma(warning(push)) \
					__pragma(warning(error:w))
		#else
			#define KM_ENABLE_VC_WARNING_AS_ERROR(w)
		#endif
	#endif

	#ifndef KM_DISABLE_VC_WARNING_AS_ERROR
		#if defined(_MSC_VER)
			#define KM_DISABLE_VC_WARNING_AS_ERROR() \
				__pragma(warning(pop))
		#else
			#define KM_DISABLE_VC_WARNING_AS_ERROR()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_GCC_WARNING / KM_RESTORE_GCC_WARNING
	//
	// Example usage:
	//     // Only one warning can be ignored per statement, due to how GCC works.
	//     KM_DISABLE_GCC_WARNING(-Wuninitialized)
	//     KM_DISABLE_GCC_WARNING(-Wunused)
	//     <code>
	//     KM_RESTORE_GCC_WARNING()
	//     KM_RESTORE_GCC_WARNING()
	//
	#ifndef KM_DISABLE_GCC_WARNING
		#if defined(KM_COMPILER_GNUC)
			#define EAGCCWHELP0(x) #x
			#define EAGCCWHELP1(x) EAGCCWHELP0(GCC diagnostic ignored x)
			#define EAGCCWHELP2(x) EAGCCWHELP1(#x)
		#endif

		#if defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION >= 4006) // Can't test directly for __GNUC__ because some compilers lie.
			#define KM_DISABLE_GCC_WARNING(w)   \
				_Pragma("GCC diagnostic push")  \
				_Pragma(EAGCCWHELP2(w))
		#elif defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION >= 4004)
			#define KM_DISABLE_GCC_WARNING(w)   \
				_Pragma(EAGCCWHELP2(w))
		#else
			#define KM_DISABLE_GCC_WARNING(w)
		#endif
	#endif

	#ifndef KM_RESTORE_GCC_WARNING
		#if defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION >= 4006)
			#define KM_RESTORE_GCC_WARNING()    \
				_Pragma("GCC diagnostic pop")
		#else
			#define KM_RESTORE_GCC_WARNING()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_ALL_GCC_WARNINGS / KM_RESTORE_ALL_GCC_WARNINGS
	//
	// This isn't possible except via using _Pragma("GCC system_header"), though
	// that has some limitations in how it works. Another means is to manually
	// disable individual warnings within a GCC diagnostic push statement.
	// GCC doesn't have as many warnings as VC++ and EDG and so this may be feasible.
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// KM_ENABLE_GCC_WARNING_AS_ERROR / KM_DISABLE_GCC_WARNING_AS_ERROR
	//
	// Example usage:
	//     // Only one warning can be treated as an error per statement, due to how GCC works.
	//     KM_ENABLE_GCC_WARNING_AS_ERROR(-Wuninitialized)
	//     KM_ENABLE_GCC_WARNING_AS_ERROR(-Wunused)
	//     <code>
	//     KM_DISABLE_GCC_WARNING_AS_ERROR()
	//     KM_DISABLE_GCC_WARNING_AS_ERROR()
	//
	#ifndef KM_ENABLE_GCC_WARNING_AS_ERROR
		#if defined(KM_COMPILER_GNUC)
			#define EAGCCWERRORHELP0(x) #x
			#define EAGCCWERRORHELP1(x) EAGCCWERRORHELP0(GCC diagnostic error x)
			#define EAGCCWERRORHELP2(x) EAGCCWERRORHELP1(#x)
		#endif

		#if defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION >= 4006) // Can't test directly for __GNUC__ because some compilers lie.
			#define KM_ENABLE_GCC_WARNING_AS_ERROR(w)   \
				_Pragma("GCC diagnostic push")  \
				_Pragma(EAGCCWERRORHELP2(w))
		#elif defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION >= 4004)
			#define KM_DISABLE_GCC_WARNING(w)   \
				_Pragma(EAGCCWERRORHELP2(w))
		#else
			#define KM_DISABLE_GCC_WARNING(w)
		#endif
	#endif

	#ifndef KM_DISABLE_GCC_WARNING_AS_ERROR
		#if defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION >= 4006)
			#define KM_DISABLE_GCC_WARNING_AS_ERROR()    \
				_Pragma("GCC diagnostic pop")
		#else
			#define KM_DISABLE_GCC_WARNING_AS_ERROR()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_CLANG_WARNING / KM_RESTORE_CLANG_WARNING
	//
	// Example usage:
	//     // Only one warning can be ignored per statement, due to how clang works.
	//     KM_DISABLE_CLANG_WARNING(-Wuninitialized)
	//     KM_DISABLE_CLANG_WARNING(-Wunused)
	//     <code>
	//     KM_RESTORE_CLANG_WARNING()
	//     KM_RESTORE_CLANG_WARNING()
	//
	#ifndef KM_DISABLE_CLANG_WARNING
		#if defined(KM_COMPILER_CLANG) || defined(KM_COMPILER_CLANG_CL)
			#define EACLANGWHELP0(x) #x
			#define EACLANGWHELP1(x) EACLANGWHELP0(clang diagnostic ignored x)
			#define EACLANGWHELP2(x) EACLANGWHELP1(#x)

			#define KM_DISABLE_CLANG_WARNING(w)   \
				_Pragma("clang diagnostic push")  \
				_Pragma(EACLANGWHELP2(-Wunknown-warning-option))\
				_Pragma(EACLANGWHELP2(w))
		#else
			#define KM_DISABLE_CLANG_WARNING(w)
		#endif
	#endif

	#ifndef KM_RESTORE_CLANG_WARNING
		#if defined(KM_COMPILER_CLANG) || defined(KM_COMPILER_CLANG_CL)
			#define KM_RESTORE_CLANG_WARNING()    \
				_Pragma("clang diagnostic pop")
		#else
			#define KM_RESTORE_CLANG_WARNING()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_ALL_CLANG_WARNINGS / KM_RESTORE_ALL_CLANG_WARNINGS
	//
	// The situation for clang is the same as for GCC. See above.
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// KM_ENABLE_CLANG_WARNING_AS_ERROR / KM_DISABLE_CLANG_WARNING_AS_ERROR
	//
	// Example usage:
	//     // Only one warning can be treated as an error per statement, due to how clang works.
	//     KM_ENABLE_CLANG_WARNING_AS_ERROR(-Wuninitialized)
	//     KM_ENABLE_CLANG_WARNING_AS_ERROR(-Wunused)
	//     <code>
	//     KM_DISABLE_CLANG_WARNING_AS_ERROR()
	//     KM_DISABLE_CLANG_WARNING_AS_ERROR()
	//
	#ifndef KM_ENABLE_CLANG_WARNING_AS_ERROR
		#if defined(KM_COMPILER_CLANG) || defined(KM_COMPILER_CLANG_CL)
			#define EACLANGWERRORHELP0(x) #x
			#define EACLANGWERRORHELP1(x) EACLANGWERRORHELP0(clang diagnostic error x)
			#define EACLANGWERRORHELP2(x) EACLANGWERRORHELP1(#x)

			#define KM_ENABLE_CLANG_WARNING_AS_ERROR(w)   \
				_Pragma("clang diagnostic push")  \
				_Pragma(EACLANGWERRORHELP2(w))
		#else
			#define KM_DISABLE_CLANG_WARNING(w)
		#endif
	#endif

	#ifndef KM_DISABLE_CLANG_WARNING_AS_ERROR
		#if defined(KM_COMPILER_CLANG) || defined(KM_COMPILER_CLANG_CL)
			#define KM_DISABLE_CLANG_WARNING_AS_ERROR()    \
				_Pragma("clang diagnostic pop")
		#else
			#define KM_DISABLE_CLANG_WARNING_AS_ERROR()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_SN_WARNING / KM_RESTORE_SN_WARNING
	//
	// Note that we define this macro specifically for the SN compiler instead of
	// having a generic one for EDG-based compilers. The reason for this is that
	// while SN is indeed based on EDG, SN has different warning value mappings
	// and thus warning 1234 for SN is not the same as 1234 for all other EDG compilers.
	//
	// Example usage:
	//     // Currently we are limited to one warning per line.
	//     KM_DISABLE_SN_WARNING(1787)
	//     KM_DISABLE_SN_WARNING(552)
	//     <code>
	//     KM_RESTORE_SN_WARNING()
	//     KM_RESTORE_SN_WARNING()
	//
	#ifndef KM_DISABLE_SN_WARNING
			#define KM_DISABLE_SN_WARNING(w)
	#endif

	#ifndef KM_RESTORE_SN_WARNING
			#define KM_RESTORE_SN_WARNING()
	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_ALL_SN_WARNINGS / KM_RESTORE_ALL_SN_WARNINGS
	//
	// Example usage:
	//     KM_DISABLE_ALL_SN_WARNINGS()
	//     <code>
	//     KM_RESTORE_ALL_SN_WARNINGS()
	//
	#ifndef KM_DISABLE_ALL_SN_WARNINGS
			#define KM_DISABLE_ALL_SN_WARNINGS()
	#endif

	#ifndef KM_RESTORE_ALL_SN_WARNINGS
			#define KM_RESTORE_ALL_SN_WARNINGS()
	#endif



	// ------------------------------------------------------------------------
	// KM_DISABLE_GHS_WARNING / KM_RESTORE_GHS_WARNING
	//
	// Disable warnings from the Green Hills compiler.
	//
	// Example usage:
	//     KM_DISABLE_GHS_WARNING(193)
	//     KM_DISABLE_GHS_WARNING(236, 5323)
	//     <code>
	//     KM_RESTORE_GHS_WARNING()
	//     KM_RESTORE_GHS_WARNING()
	//
	#ifndef KM_DISABLE_GHS_WARNING
			#define KM_DISABLE_GHS_WARNING(w)
	#endif

	#ifndef KM_RESTORE_GHS_WARNING
			#define KM_RESTORE_GHS_WARNING()
	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_ALL_GHS_WARNINGS / KM_RESTORE_ALL_GHS_WARNINGS
	//
	// #ifndef KM_DISABLE_ALL_GHS_WARNINGS
	//     #if defined(KM_COMPILER_GREEN_HILLS)
	//         #define KM_DISABLE_ALL_GHS_WARNINGS(w)  \_
	//             _Pragma("_________")
	//     #else
	//         #define KM_DISABLE_ALL_GHS_WARNINGS(w)
	//     #endif
	// #endif
	// 
	// #ifndef KM_RESTORE_ALL_GHS_WARNINGS
	//     #if defined(KM_COMPILER_GREEN_HILLS)
	//         #define KM_RESTORE_ALL_GHS_WARNINGS()   \_
	//             _Pragma("_________")
	//     #else
	//         #define KM_RESTORE_ALL_GHS_WARNINGS()
	//     #endif
	// #endif



	// ------------------------------------------------------------------------
	// KM_DISABLE_EDG_WARNING / KM_RESTORE_EDG_WARNING
	//
	// Example usage:
	//     // Currently we are limited to one warning per line.
	//     KM_DISABLE_EDG_WARNING(193)
	//     KM_DISABLE_EDG_WARNING(236)
	//     <code>
	//     KM_RESTORE_EDG_WARNING()
	//     KM_RESTORE_EDG_WARNING()
	//
	#ifndef KM_DISABLE_EDG_WARNING
		// EDG-based compilers are inconsistent in how the implement warning pragmas.
		#if defined(KM_COMPILER_EDG) && !defined(KM_COMPILER_INTEL) && !defined(KM_COMPILER_RVCT)
			#define EAEDGWHELP0(x) #x
			#define EAEDGWHELP1(x) EAEDGWHELP0(diag_suppress x)

			#define KM_DISABLE_EDG_WARNING(w)   \
				_Pragma("control %push diag")   \
				_Pragma(EAEDGWHELP1(w))
		#else
			#define KM_DISABLE_EDG_WARNING(w)
		#endif
	#endif

	#ifndef KM_RESTORE_EDG_WARNING
		#if defined(KM_COMPILER_EDG) && !defined(KM_COMPILER_INTEL) && !defined(KM_COMPILER_RVCT)
			#define KM_RESTORE_EDG_WARNING()   \
				_Pragma("control %pop diag")
		#else
			#define KM_RESTORE_EDG_WARNING()
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_ALL_EDG_WARNINGS / KM_RESTORE_ALL_EDG_WARNINGS
	//
	//#ifndef KM_DISABLE_ALL_EDG_WARNINGS
	//    #if defined(KM_COMPILER_EDG) && !defined(KM_COMPILER_SN)
	//        #define KM_DISABLE_ALL_EDG_WARNINGS(w)  \_
	//            _Pragma("_________")
	//    #else
	//        #define KM_DISABLE_ALL_EDG_WARNINGS(w)
	//    #endif
	//#endif
	//
	//#ifndef KM_RESTORE_ALL_EDG_WARNINGS
	//    #if defined(KM_COMPILER_EDG) && !defined(KM_COMPILER_SN)
	//        #define KM_RESTORE_ALL_EDG_WARNINGS()   \_
	//            _Pragma("_________")
	//    #else
	//        #define KM_RESTORE_ALL_EDG_WARNINGS()
	//    #endif
	//#endif



	// ------------------------------------------------------------------------
	// KM_DISABLE_CW_WARNING / KM_RESTORE_CW_WARNING
	//
	// Note that this macro can only control warnings via numbers and not by 
	// names. The reason for this is that the compiler's syntax for such 
	// warnings is not the same as for numbers.
	// 
	// Example usage:
	//     // Currently we are limited to one warning per line and must also specify the warning in the restore macro.
	//     KM_DISABLE_CW_WARNING(10317)
	//     KM_DISABLE_CW_WARNING(10324)
	//     <code>
	//     KM_RESTORE_CW_WARNING(10317)
	//     KM_RESTORE_CW_WARNING(10324)
	//
	#ifndef KM_DISABLE_CW_WARNING
		#define KM_DISABLE_CW_WARNING(w)
	#endif

	#ifndef KM_RESTORE_CW_WARNING

		#define KM_RESTORE_CW_WARNING(w)

	#endif


	// ------------------------------------------------------------------------
	// KM_DISABLE_ALL_CW_WARNINGS / KM_RESTORE_ALL_CW_WARNINGS
	//
	#ifndef KM_DISABLE_ALL_CW_WARNINGS
		#define KM_DISABLE_ALL_CW_WARNINGS()

	#endif
	
	#ifndef KM_RESTORE_ALL_CW_WARNINGS
		#define KM_RESTORE_ALL_CW_WARNINGS()
	#endif



	// ------------------------------------------------------------------------
	// KM_PURE
	// 
	// This acts the same as the GCC __attribute__ ((pure)) directive and is
	// implemented simply as a wrapper around it to allow portable usage of 
	// it and to take advantage of it if and when it appears in other compilers.
	//
	// A "pure" function is one that has no effects except its return value and 
	// its return value is a function of only the function's parameters or 
	// non-volatile global variables. Any parameter or global variable access 
	// must be read-only. Loop optimization and subexpression elimination can be 
	// applied to such functions. A common example is strlen(): Given identical 
	// inputs, the function's return value (its only effect) is invariant across 
	// multiple invocations and thus can be pulled out of a loop and called but once.
	//
	// Example usage:
	//    KM_PURE void Function();
	//
	#ifndef KM_PURE
		#if defined(KM_COMPILER_GNUC)
			#define KM_PURE __attribute__((pure))
		#elif defined(KM_COMPILER_ARM)  // Arm brand compiler for ARM CPU
			#define KM_PURE __pure
		#else
			#define KM_PURE
		#endif
	#endif



	// ------------------------------------------------------------------------
	// KM_WEAK
	// KM_WEAK_SUPPORTED -- defined as 0 or 1.
	// 
	// GCC
	// The weak attribute causes the declaration to be emitted as a weak
	// symbol rather than a global. This is primarily useful in defining
	// library functions which can be overridden in user code, though it
	// can also be used with non-function declarations.
	//
	// VC++
	// At link time, if multiple definitions of a COMDAT are seen, the linker 
	// picks one and discards the rest. If the linker option /OPT:REF 
	// is selected, then COMDAT elimination will occur to remove all the 
	// unreferenced data items in the linker output.
	//
	// Example usage:
	//    KM_WEAK void Function();
	//
	#ifndef KM_WEAK
		#if defined(_MSC_VER) && (_MSC_VER >= 1300) // If VC7.0 and later 
			#define KM_WEAK __declspec(selectany)
			#define KM_WEAK_SUPPORTED 1
		#elif defined(_MSC_VER) || (defined(__GNUC__) && defined(__CYGWIN__))
			#define KM_WEAK
			#define KM_WEAK_SUPPORTED 0
		#elif defined(KM_COMPILER_ARM)  // Arm brand compiler for ARM CPU
			#define KM_WEAK __weak
			#define KM_WEAK_SUPPORTED 1
		#else                           // GCC and IBM compilers, others.
			#define KM_WEAK __attribute__((weak))
			#define KM_WEAK_SUPPORTED 1
		#endif
	#endif



	// ------------------------------------------------------------------------
	// KM_UNUSED
	// 
	// Makes compiler warnings about unused variables go away.
	//
	// Example usage:
	//    void Function(int x)
	//    {
	//        int y;
	//        KM_UNUSED(x);
	//        KM_UNUSED(y);
	//    }
	//
	#ifndef KM_UNUSED
		// The EDG solution below is pretty weak and needs to be augmented or replaced.
		// It can't handle the C language, is limited to places where template declarations
		// can be used, and requires the type x to be usable as a functions reference argument. 
		#if defined(__cplusplus) && defined(__EDG__)
			template <typename T>
			inline void EABaseUnused(T const volatile & x) { (void)x; }
			#define KM_UNUSED(x) EABaseUnused(x)
		#else
			#define KM_UNUSED(x) (void)x
		#endif
	#endif



	// ------------------------------------------------------------------------
	// KM_EMPTY
	// 
	// Allows for a null statement, usually for the purpose of avoiding compiler warnings.
	//
	// Example usage:
	//    #ifdef KM_DEBUG
	//        #define MyDebugPrintf(x, y) printf(x, y)
	//    #else
	//        #define MyDebugPrintf(x, y)  KM_EMPTY
	//    #endif
	//
	#ifndef KM_EMPTY
		#define KM_EMPTY (void)0
	#endif


	// ------------------------------------------------------------------------
	// KM_CURRENT_FUNCTION
	//
	// Provides a consistent way to get the current function name as a macro
	// like the __FILE__ and __LINE__ macros work. The C99 standard specifies
	// that __func__ be provided by the compiler, but most compilers don't yet
	// follow that convention. However, many compilers have an alternative.
	//
	// We also define KM_CURRENT_FUNCTION_SUPPORTED for when it is not possible
	// to have KM_CURRENT_FUNCTION work as expected.
	//
	// Defined inside a function because otherwise the macro might not be 
	// defined and code below might not compile. This happens with some 
	// compilers.
	//
	#ifndef KM_CURRENT_FUNCTION
		#if defined __GNUC__ || (defined __ICC && __ICC >= 600)
			#define KM_CURRENT_FUNCTION __PRETTY_FUNCTION__
		#elif defined(__FUNCSIG__)
			#define KM_CURRENT_FUNCTION __FUNCSIG__
		#elif (defined __INTEL_COMPILER && __INTEL_COMPILER >= 600) || (defined __IBMCPP__ && __IBMCPP__ >= 500) || (defined CS_UNDEFINED_STRING && CS_UNDEFINED_STRING >= 0x4200)
			#define KM_CURRENT_FUNCTION __FUNCTION__
		#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901
			#define KM_CURRENT_FUNCTION __func__
		#else
			#define KM_CURRENT_FUNCTION "(unknown function)"
		#endif
	#endif


	// ------------------------------------------------------------------------
	// wchar_t
	// Here we define:
	//    KM_WCHAR_T_NON_NATIVE
	//    KM_WCHAR_SIZE = <sizeof(wchar_t)>
	//
	#ifndef KM_WCHAR_T_NON_NATIVE
		// Compilers that always implement wchar_t as native include:
		//     COMEAU, new SN, and other EDG-based compilers.
		//     GCC
		//     Borland
		//     SunPro
		//     IBM Visual Age
		#if defined(KM_COMPILER_INTEL)
			#if (KM_COMPILER_VERSION < 700)
				#define KM_WCHAR_T_NON_NATIVE 1
			#else
				#if (!defined(_WCHAR_T_DEFINED) && !defined(_WCHAR_T))
					#define KM_WCHAR_T_NON_NATIVE 1
				#endif
			#endif
		#elif defined(KM_COMPILER_MSVC) || (defined(KM_COMPILER_CLANG) && defined(KM_PLATFORM_WINDOWS))
			#ifndef _NATIVE_WCHAR_T_DEFINED
				#define KM_WCHAR_T_NON_NATIVE 1
			#endif
		#elif defined(__EDG_VERSION__) && (!defined(_WCHAR_T) && (__EDG_VERSION__ < 400)) // EDG prior to v4 uses _WCHAR_T to indicate if wchar_t is native. v4+ may define something else, but we're not currently aware of it.
			#define KM_WCHAR_T_NON_NATIVE 1
		#endif
	#endif

	#ifndef KM_WCHAR_SIZE // If the user hasn't specified that it is a given size...
		#if defined(__WCHAR_MAX__) // GCC defines this for most platforms.
			#if (__WCHAR_MAX__ == 2147483647) || (__WCHAR_MAX__ == 4294967295)
				#define KM_WCHAR_SIZE 4
			#elif (__WCHAR_MAX__ == 32767) || (__WCHAR_MAX__ == 65535)
				#define KM_WCHAR_SIZE 2
			#elif (__WCHAR_MAX__ == 127) || (__WCHAR_MAX__ == 255)
				#define KM_WCHAR_SIZE 1
			#else
				#define KM_WCHAR_SIZE 4
			#endif
		#elif defined(WCHAR_MAX) // The SN and Arm compilers define this.
			#if (WCHAR_MAX == 2147483647) || (WCHAR_MAX == 4294967295)
				#define KM_WCHAR_SIZE 4
			#elif (WCHAR_MAX == 32767) || (WCHAR_MAX == 65535)
				#define KM_WCHAR_SIZE 2
			#elif (WCHAR_MAX == 127) || (WCHAR_MAX == 255)
				#define KM_WCHAR_SIZE 1
			#else
				#define KM_WCHAR_SIZE 4
			#endif
		#elif defined(__WCHAR_BIT) // Green Hills (and other versions of EDG?) uses this.
			#if (__WCHAR_BIT == 16)
				#define KM_WCHAR_SIZE 2
			#elif (__WCHAR_BIT == 32)
				#define KM_WCHAR_SIZE 4
			#elif (__WCHAR_BIT == 8)
				#define KM_WCHAR_SIZE 1
			#else
				#define KM_WCHAR_SIZE 4
			#endif
		#elif defined(_WCMAX) // The SN and Arm compilers define this.
			#if (_WCMAX == 2147483647) || (_WCMAX == 4294967295)
				#define KM_WCHAR_SIZE 4
			#elif (_WCMAX == 32767) || (_WCMAX == 65535)
				#define KM_WCHAR_SIZE 2
			#elif (_WCMAX == 127) || (_WCMAX == 255)
				#define KM_WCHAR_SIZE 1
			#else
				#define KM_WCHAR_SIZE 4
			#endif
		#elif defined(KM_PLATFORM_UNIX)
			// It is standard on Unix to have wchar_t be int32_t or uint32_t.
			// All versions of GNUC default to a 32 bit wchar_t, but EA has used 
			// the -fshort-wchar GCC command line option to force it to 16 bit.
			// If you know that the compiler is set to use a wchar_t of other than 
			// the default, you need to manually define KM_WCHAR_SIZE for the build.
			#define KM_WCHAR_SIZE 4
		#else
			// It is standard on Windows to have wchar_t be uint16_t.  GCC
			// defines wchar_t as int by default.  Electronic Arts has
			// standardized on wchar_t being an unsigned 16 bit value on all
			// console platforms. Given that there is currently no known way to
			// tell at preprocessor time what the size of wchar_t is, we declare
			// it to be 2, as this is the Electronic Arts standard. If you have
			// KM_WCHAR_SIZE != sizeof(wchar_t), then your code might not be
			// broken, but it also won't work with wchar libraries and data from
			// other parts of EA. Under GCC, you can force wchar_t to two bytes
			// with the -fshort-wchar compiler argument.
			#define KM_WCHAR_SIZE 2
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_RESTRICT
	// 
	// The C99 standard defines a new keyword, restrict, which allows for the 
	// improvement of code generation regarding memory usage. Compilers can
	// generate significantly faster code when you are able to use restrict.
	// 
	// Example usage:
	//    void DoSomething(char* KM_RESTRICT p1, char* KM_RESTRICT p2);
	//
	#ifndef KM_RESTRICT
		#if defined(KM_COMPILER_MSVC) && (KM_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
			#define KM_RESTRICT __restrict
		#elif defined(KM_COMPILER_CLANG)
			#define KM_RESTRICT __restrict
		#elif defined(KM_COMPILER_GNUC)     // Includes GCC and other compilers emulating GCC.
			#define KM_RESTRICT __restrict  // GCC defines 'restrict' (as opposed to __restrict) in C99 mode only.
		#elif defined(KM_COMPILER_ARM)
			#define KM_RESTRICT __restrict
		#elif defined(KM_COMPILER_IS_C99)
			#define KM_RESTRICT restrict
		#else
			// If the compiler didn't support restricted pointers, defining KM_RESTRICT
			// away would result in compiling and running fine but you just wouldn't 
			// the same level of optimization. On the other hand, all the major compilers 
			// support restricted pointers.
			#define KM_RESTRICT
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_DEPRECATED            // Used as a prefix.
	// KM_PREFIX_DEPRECATED     // You should need this only for unusual compilers.
	// KM_POSTFIX_DEPRECATED    // You should need this only for unusual compilers.
	// KM_DEPRECATED_MESSAGE    // Used as a prefix and provides a deprecation message.
	// 
	// Example usage:
	//    KM_DEPRECATED void Function();
	//    KM_DEPRECATED_MESSAGE("Use 1.0v API instead") void Function();
	//
	// or for maximum portability:
	//    KM_PREFIX_DEPRECATED void Function() KM_POSTFIX_DEPRECATED;
	//

	#ifndef KM_DEPRECATED
		#if defined(KM_COMPILER_CPP14_ENABLED)
			#define KM_DEPRECATED [[deprecated]]
		#elif defined(KM_COMPILER_MSVC) && (KM_COMPILER_VERSION > 1300) // If VC7 (VS2003) or later...
			#define KM_DEPRECATED __declspec(deprecated)
		#elif defined(KM_COMPILER_MSVC)
			#define KM_DEPRECATED
		#else
			#define KM_DEPRECATED __attribute__((deprecated))
		#endif
	#endif

	#ifndef KM_PREFIX_DEPRECATED
		#if defined(KM_COMPILER_CPP14_ENABLED)
			#define KM_PREFIX_DEPRECATED [[deprecated]]
			#define KM_POSTFIX_DEPRECATED
		#elif defined(KM_COMPILER_MSVC) && (KM_COMPILER_VERSION > 1300) // If VC7 (VS2003) or later...
			#define KM_PREFIX_DEPRECATED __declspec(deprecated)
			#define KM_POSTFIX_DEPRECATED
		#elif defined(KM_COMPILER_MSVC)
			#define KM_PREFIX_DEPRECATED
			#define KM_POSTFIX_DEPRECATED
		#else
			#define KM_PREFIX_DEPRECATED
			#define KM_POSTFIX_DEPRECATED __attribute__((deprecated))
		#endif
	#endif

	#ifndef KM_DEPRECATED_MESSAGE
		#if defined(KM_COMPILER_CPP14_ENABLED)
			#define KM_DEPRECATED_MESSAGE(msg) [[deprecated(#msg)]]
		#else
			// Compiler does not support depreaction messages, explicitly drop the msg but still mark the function as deprecated
			#define KM_DEPRECATED_MESSAGE(msg) KM_DEPRECATED
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_FORCE_INLINE              // Used as a prefix.
	// KM_PREFIX_FORCE_INLINE       // You should need this only for unusual compilers.
	// KM_POSTFIX_FORCE_INLINE      // You should need this only for unusual compilers.
	//
	// Example usage:
	//     KM_FORCE_INLINE void Foo();                                // Implementation elsewhere.
	//     KM_PREFIX_FORCE_INLINE void Foo() KM_POSTFIX_FORCE_INLINE; // Implementation elsewhere.
	//
	// Note that when the prefix version of this function is used, it replaces
	// the regular C++ 'inline' statement. Thus you should not use both the 
	// C++ inline statement and this macro with the same function declaration.
	//
	// To force inline usage under GCC 3.1+, you use this:
	//    inline void Foo() __attribute__((always_inline));
	//       or
	//    inline __attribute__((always_inline)) void Foo();
	//
	// The CodeWarrior compiler doesn't have the concept of forcing inlining per function.
	// 
	#ifndef KM_FORCE_INLINE
		#if defined(KM_COMPILER_MSVC)
			#define KM_FORCE_INLINE __forceinline
		#elif defined(KM_COMPILER_GNUC) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 301) || defined(KM_COMPILER_CLANG)
			#if defined(__cplusplus)
				#define KM_FORCE_INLINE inline __attribute__((always_inline))
			#else
				#define KM_FORCE_INLINE __inline__ __attribute__((always_inline))
			#endif
		#else
			#if defined(__cplusplus)
				#define KM_FORCE_INLINE inline
			#else
				#define KM_FORCE_INLINE __inline
			#endif
		#endif
	#endif

	#if   defined(KM_COMPILER_GNUC) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 301) || defined(KM_COMPILER_CLANG)
		#define KM_PREFIX_FORCE_INLINE  inline
		#define KM_POSTFIX_FORCE_INLINE __attribute__((always_inline))
	#else
		#define KM_PREFIX_FORCE_INLINE  inline
		#define KM_POSTFIX_FORCE_INLINE
	#endif


	// ------------------------------------------------------------------------
	// KM_FORCE_INLINE_LAMBDA
	//
	// KM_FORCE_INLINE_LAMBDA is used to force inline a call to a lambda when possible.
	// Force inlining a lambda can be useful to reduce overhead in situations where a lambda may
	// may only be called once, or inlining allows the compiler to apply other optimizations that wouldn't
	// otherwise be possible.
	//
	// The ability to force inline a lambda is currently only available on a subset of compilers.
	//
	// Example usage:
	//
	//		auto lambdaFunction = []() KM_FORCE_INLINE_LAMBDA
	//		{
	//		};
	//
	#ifndef KM_FORCE_INLINE_LAMBDA
		#if defined(KM_COMPILER_GNUC) || defined(KM_COMPILER_CLANG)
			#define KM_FORCE_INLINE_LAMBDA __attribute__((always_inline))
		#else
			#define KM_FORCE_INLINE_LAMBDA
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_NO_INLINE             // Used as a prefix.
	// KM_PREFIX_NO_INLINE      // You should need this only for unusual compilers.
	// KM_POSTFIX_NO_INLINE     // You should need this only for unusual compilers.
	//
	// Example usage:
	//     KM_NO_INLINE        void Foo();                       // Implementation elsewhere.
	//     KM_PREFIX_NO_INLINE void Foo() KM_POSTFIX_NO_INLINE;  // Implementation elsewhere.
	//
	// That this declaration is incompatbile with C++ 'inline' and any
	// variant of KM_FORCE_INLINE.
	//
	// To disable inline usage under VC++ priof to VS2005, you need to use this:
	//    #pragma inline_depth(0) // Disable inlining.
	//    void Foo() { ... }
	//    #pragma inline_depth()  // Restore to default.
	//
	// Since there is no easy way to disable inlining on a function-by-function
	// basis in VC++ prior to VS2005, the best strategy is to write platform-specific 
	// #ifdefs in the code or to disable inlining for a given module and enable 
	// functions individually with KM_FORCE_INLINE.
	// 
	#ifndef KM_NO_INLINE
		#if defined(KM_COMPILER_MSVC) && (KM_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
			#define KM_NO_INLINE __declspec(noinline)
		#elif defined(KM_COMPILER_MSVC)
			#define KM_NO_INLINE
		#else
			#define KM_NO_INLINE __attribute__((noinline))
		#endif
	#endif

	#if defined(KM_COMPILER_MSVC) && (KM_COMPILER_VERSION >= 1400) // If VC8 (VS2005) or later...
		#define KM_PREFIX_NO_INLINE  __declspec(noinline)
		#define KM_POSTFIX_NO_INLINE
	#elif defined(KM_COMPILER_MSVC)
		#define KM_PREFIX_NO_INLINE
		#define KM_POSTFIX_NO_INLINE
	#else
		#define KM_PREFIX_NO_INLINE
		#define KM_POSTFIX_NO_INLINE __attribute__((noinline))
	#endif


	// ------------------------------------------------------------------------
	// KM_NO_VTABLE
	//
	// Example usage:
	//     class KM_NO_VTABLE X {
	//        virtual void InterfaceFunction();
	//     };
	// 
	//     KM_CLASS_NO_VTABLE(X) {
	//        virtual void InterfaceFunction();
	//     };
	//
	#ifdef KM_COMPILER_MSVC
		#define KM_NO_VTABLE           __declspec(novtable)
		#define KM_CLASS_NO_VTABLE(x)  class __declspec(novtable) x
		#define KM_STRUCT_NO_VTABLE(x) struct __declspec(novtable) x
	#else
		#define KM_NO_VTABLE
		#define KM_CLASS_NO_VTABLE(x)  class x
		#define KM_STRUCT_NO_VTABLE(x) struct x
	#endif


	// ------------------------------------------------------------------------
	// KM_PASCAL
	//
	// Also known on PC platforms as stdcall.
	// This convention causes the compiler to assume that the called function 
	// will pop off the stack space used to pass arguments, unless it takes a 
	// variable number of arguments. 
	//
	// Example usage:
	//    this:
	//       void DoNothing(int x);
	//       void DoNothing(int x){}
	//    would be written as this:
	//       void KM_PASCAL_FUNC(DoNothing(int x));
	//       void KM_PASCAL_FUNC(DoNothing(int x)){}
	// 
	#ifndef KM_PASCAL
		#if defined(KM_COMPILER_MSVC)
			#define KM_PASCAL __stdcall
		#elif defined(KM_COMPILER_GNUC) && defined(KM_PROCESSOR_X86)
			#define KM_PASCAL __attribute__((stdcall))
		#else
			// Some compilers simply don't support pascal calling convention.
			// As a result, there isn't an issue here, since the specification of 
			// pascal calling convention is for the purpose of disambiguating the
			// calling convention that is applied.
			#define KM_PASCAL
		#endif
	#endif

	#ifndef KM_PASCAL_FUNC
		#if defined(KM_COMPILER_MSVC)
			#define KM_PASCAL_FUNC(funcname_and_paramlist)    __stdcall funcname_and_paramlist
		#elif defined(KM_COMPILER_GNUC) && defined(KM_PROCESSOR_X86)
			#define KM_PASCAL_FUNC(funcname_and_paramlist)    __attribute__((stdcall)) funcname_and_paramlist
		#else
			#define KM_PASCAL_FUNC(funcname_and_paramlist)    funcname_and_paramlist
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_SSE
	// Visual C Processor Packs define _MSC_FULL_VER and are needed for SSE
	// Intel C also has SSE support.
	// KM_SSE is used to select FPU or SSE versions in hw_select.inl
	//
	// KM_SSE defines the level of SSE support:
	//  0 indicates no SSE support
	//  1 indicates SSE1 is supported
	//  2 indicates SSE2 is supported
	//  3 indicates SSE3 (or greater) is supported
	//
	// Note: SSE support beyond SSE3 can't be properly represented as a single
	// version number.  Instead users should use specific SSE defines (e.g.
	// KM_SSE4_2) to detect what specific support is available.  KM_SSE being
	// equal to 3 really only indicates that SSE3 or greater is supported.
	#ifndef KM_SSE
		#if defined(KM_COMPILER_GNUC) || defined(KM_COMPILER_CLANG)
			#if defined(__SSE3__)
				#define KM_SSE 3
			#elif defined(__SSE2__)
				#define KM_SSE 2
			#elif defined(__SSE__) && __SSE__
				#define KM_SSE 1
			#else
				#define KM_SSE 0
			#endif
		#elif (defined(KM_SSE3) && KM_SSE3) || defined KM_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define KM_SSE 3
		#elif defined(KM_SSE2) && KM_SSE2
			#define KM_SSE 2
		#elif defined(KM_PROCESSOR_X86) && defined(_MSC_FULL_VER) && !defined(__NOSSE__) && defined(_M_IX86_FP)
			#define KM_SSE _M_IX86_FP
		#elif defined(KM_PROCESSOR_X86) && defined(KM_COMPILER_INTEL) && !defined(__NOSSE__)
			#define KM_SSE 1
		#elif defined(KM_PROCESSOR_X86_64)
			// All x64 processors support SSE2 or higher
			#define KM_SSE 2
		#else
			#define KM_SSE 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// We define separate defines for SSE support beyond SSE1.  These defines
	// are particularly useful for detecting SSE4.x features since there isn't
	// a single concept of SSE4.
	//
	// The following SSE defines are always defined.  0 indicates the
	// feature/level of SSE is not supported, and 1 indicates support is
	// available.
	#ifndef KM_SSE2
		#if KM_SSE >= 2
			#define KM_SSE2 1
		#else
			#define KM_SSE2 0
		#endif
	#endif
	#ifndef KM_SSE3
		#if KM_SSE >= 3
			#define KM_SSE3 1
		#else
			#define KM_SSE3 0
		#endif
	#endif
	#ifndef KM_SSSE3
		#if defined __SSSE3__ || defined KM_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define KM_SSSE3 1
		#else
			#define KM_SSSE3 0
		#endif
	#endif
	#ifndef KM_SSE4_1
		#if defined __SSE4_1__ || defined KM_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define KM_SSE4_1 1
		#else
			#define KM_SSE4_1 0
		#endif
	#endif
	#ifndef KM_SSE4_2
		#if defined __SSE4_2__ || defined KM_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define KM_SSE4_2 1
		#else
			#define KM_SSE4_2 0
		#endif
	#endif
	#ifndef KM_SSE4A
		#if defined __SSE4A__ || defined KM_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define KM_SSE4A 1
		#else
			#define KM_SSE4A 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// KM_AVX
	// KM_AVX may be used to determine if Advanced Vector Extensions are available for the target architecture
	//
	// KM_AVX defines the level of AVX support:
	//  0 indicates no AVX support
	//  1 indicates AVX1 is supported
	//  2 indicates AVX2 is supported
	#ifndef KM_AVX
		#if defined __AVX2__
			#define KM_AVX 2
		#elif defined __AVX__ || defined KM_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define KM_AVX 1
		#else
			#define KM_AVX 0
		#endif
	#endif
	#ifndef KM_AVX2
		#if KM_AVX >= 2
			#define KM_AVX2 1
		#else
			#define KM_AVX2 0
		#endif
	#endif

	// KM_FP16C may be used to determine the existence of float <-> half conversion operations on an x86 CPU.
	// (For example to determine if _mm_cvtph_ps or _mm_cvtps_ph could be used.)
	#ifndef KM_FP16C
		#if defined __F16C__ || defined KM_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
			#define KM_FP16C 1
		#else
			#define KM_FP16C 0
		#endif
	#endif

	// KM_FP128 may be used to determine if __float128 is a supported type for use. This type is enabled by a GCC extension (_GLIBCXX_USE_FLOAT128)
	// but has support by some implementations of clang (__FLOAT128__)
	// PS4 does not support __float128 as of SDK 5.500 https://ps4.siedev.net/resources/documents/SDK/5.500/CPU_Compiler_ABI-Overview/0003.html
	#ifndef KM_FP128
		#if (defined __FLOAT128__ || defined _GLIBCXX_USE_FLOAT128) && !defined(KM_PLATFORM_SONY)
			#define KM_FP128 1
		#else
			#define KM_FP128 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// KM_ABM
	// KM_ABM may be used to determine if Advanced Bit Manipulation sets are available for the target architecture (POPCNT, LZCNT)
	// 
	#ifndef KM_ABM
		#if defined(__ABM__) || defined(KM_PLATFORM_XBOXONE) || defined(KM_PLATFORM_SONY) || defined(CS_UNDEFINED_STRING)
			#define KM_ABM 1
		#else
			#define KM_ABM 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// KM_NEON
	// KM_NEON may be used to determine if NEON is supported.
	#ifndef KM_NEON
		#if defined(__ARM_NEON__) || defined(__ARM_NEON)
			#define KM_NEON 1
		#else
			#define KM_NEON 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// KM_BMI
	// KM_BMI may be used to determine if Bit Manipulation Instruction sets are available for the target architecture
	//
	// KM_BMI defines the level of BMI support:
	//  0 indicates no BMI support
	//  1 indicates BMI1 is supported
	//  2 indicates BMI2 is supported
	#ifndef KM_BMI
		#if defined(__BMI2__)
			#define KM_BMI 2
		#elif defined(__BMI__) || defined(KM_PLATFORM_XBOXONE) || defined(CS_UNDEFINED_STRING)
			#define KM_BMI 1
		#else
			#define KM_BMI 0
		#endif
	#endif
	#ifndef KM_BMI2
		#if KM_BMI >= 2
			#define KM_BMI2 1
		#else
			#define KM_BMI2 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// KM_FMA3
	// KM_FMA3 may be used to determine if Fused Multiply Add operations are available for the target architecture
	// __FMA__ is defined only by GCC, Clang, and ICC; MSVC only defines __AVX__ and __AVX2__
	// FMA3 was introduced alongside AVX2 on Intel Haswell
	// All AMD processors support FMA3 if AVX2 is also supported
	//
	// KM_FMA3 defines the level of FMA3 support:
	//  0 indicates no FMA3 support
	//  1 indicates FMA3 is supported
	#ifndef KM_FMA3
		#if defined(__FMA__) || KM_AVX2 >= 1
			#define KM_FMA3 1
		#else
			#define KM_FMA3 0
		#endif
	#endif

	// ------------------------------------------------------------------------
	// KM_TBM
	// KM_TBM may be used to determine if Trailing Bit Manipulation instructions are available for the target architecture
	#ifndef KM_TBM
		#if defined(__TBM__)
			#define KM_TBM 1
		#else
			#define KM_TBM 0
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_IMPORT
	// import declaration specification
	// specifies that the declared symbol is imported from another dynamic library.
	#ifndef KM_IMPORT
		#if defined(KM_COMPILER_MSVC)
			#define KM_IMPORT __declspec(dllimport)
		#else
			#define KM_IMPORT
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_EXPORT
	// export declaration specification
	// specifies that the declared symbol is exported from the current dynamic library.
	// this is not the same as the C++ export keyword. The C++ export keyword has been
	// removed from the language as of C++11.
	#ifndef KM_EXPORT
		#if defined(KM_COMPILER_MSVC)
			#define KM_EXPORT __declspec(dllexport)
		#else
			#define KM_EXPORT
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_PRAGMA_ONCE_SUPPORTED
	// 
	// This is a wrapper for the #pragma once preprocessor directive.
	// It allows for some compilers (in particular VC++) to implement signifcantly
	// faster include file preprocessing. #pragma once can be used to replace 
	// header include guards or to augment them. However, #pragma once isn't 
	// necessarily supported by all compilers and isn't guaranteed to be so in
	// the future, so using #pragma once to replace traditional include guards 
	// is not strictly portable. Note that a direct #define for #pragma once is
	// impossible with VC++, due to limitations, but can be done with other 
	// compilers/preprocessors via _Pragma("once").
	// 
	// Example usage (which includes traditional header guards for portability):
	//    #ifndef SOMEPACKAGE_SOMEHEADER_H
	//    #define SOMEPACKAGE_SOMEHEADER_H
	//
	//    #if defined(KM_PRAGMA_ONCE_SUPPORTED)
	//        #pragma once
	//    #endif
	//
	//    <user code> 
	//
	//    #endif
	//
	#if defined(_MSC_VER) || defined(__GNUC__) || defined(__EDG__) || defined(__APPLE__)
		#define KM_PRAGMA_ONCE_SUPPORTED 1
	#endif



	// ------------------------------------------------------------------------
	// KM_ONCE
	// 
	// Example usage (which includes traditional header guards for portability):
	//    #ifndef SOMEPACKAGE_SOMEHEADER_H
	//    #define SOMEPACKAGE_SOMEHEADER_H
	//
	//    KM_ONCE()
	//
	//    <user code> 
	//
	//    #endif
	//
	#if defined(KM_PRAGMA_ONCE_SUPPORTED)
		#if defined(_MSC_VER)
			#define KM_ONCE() __pragma(once)
		#else
			#define KM_ONCE() // _Pragma("once")   It turns out that _Pragma("once") isn't supported by many compilers.
		#endif
	#endif



	// ------------------------------------------------------------------------
	// KM_OVERRIDE
	// 
	// C++11 override
	// See http://msdn.microsoft.com/en-us/library/jj678987.aspx for more information.
	// You can use KM_FINAL_OVERRIDE to combine usage of KM_OVERRIDE and KM_INHERITANCE_FINAL in a single statement.
	//
	// Example usage: 
	//        struct B     { virtual void f(int); };
	//        struct D : B { void f(int) KM_OVERRIDE; };
	// 
	#ifndef KM_OVERRIDE
		#if defined(KM_COMPILER_NO_OVERRIDE)
			#define KM_OVERRIDE
		#else
			#define KM_OVERRIDE override
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_INHERITANCE_FINAL
	// 
	// Portably wraps the C++11 final specifier.
	// See http://msdn.microsoft.com/en-us/library/jj678985.aspx for more information.
	// You can use KM_FINAL_OVERRIDE to combine usage of KM_OVERRIDE and KM_INHERITANCE_FINAL in a single statement.
	// This is not called KM_FINAL because that term is used within EA to denote debug/release/final builds.
	//
	// Example usage:
	//     struct B { virtual void f() KM_INHERITANCE_FINAL; };
	// 
	#ifndef KM_INHERITANCE_FINAL
		#if defined(KM_COMPILER_NO_INHERITANCE_FINAL)
			#define KM_INHERITANCE_FINAL
		#elif (defined(_MSC_VER) && (KM_COMPILER_VERSION < 1700))  // Pre-VS2012
			#define KM_INHERITANCE_FINAL sealed
		#else
			#define KM_INHERITANCE_FINAL final
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_FINAL_OVERRIDE
	// 
	// Portably wraps the C++11 override final specifiers combined.
	//
	// Example usage:
	//     struct A            { virtual void f(); };
	//     struct B : public A { virtual void f() KM_FINAL_OVERRIDE; };
	// 
	#ifndef KM_FINAL_OVERRIDE
		#define KM_FINAL_OVERRIDE KM_OVERRIDE KM_INHERITANCE_FINAL
	#endif


	// ------------------------------------------------------------------------
	// KM_SEALED
	// 
	// This is deprecated, as the C++11 Standard has final (KM_INHERITANCE_FINAL) instead.
	// See http://msdn.microsoft.com/en-us/library/0w2w91tf.aspx for more information.
	// Example usage:
	//     struct B { virtual void f() KM_SEALED; };
	// 
	#ifndef KM_SEALED
		#if defined(KM_COMPILER_MSVC) && (KM_COMPILER_VERSION >= 1400) // VS2005 (VC8) and later
			#define KM_SEALED sealed
		#else
			#define KM_SEALED
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_ABSTRACT
	// 
	// This is a Microsoft language extension.
	// See http://msdn.microsoft.com/en-us/library/b0z6b513.aspx for more information.
	// Example usage:
	//     struct X KM_ABSTRACT { virtual void f(){} };
	// 
	#ifndef KM_ABSTRACT
		#if defined(KM_COMPILER_MSVC) && (KM_COMPILER_VERSION >= 1400) // VS2005 (VC8) and later
			#define KM_ABSTRACT abstract
		#else
			#define KM_ABSTRACT
		#endif
	#endif

	/////////////////////////////////////////////////////////////////////////////////
	//
	// KM_CPP20_CONSTEXPR
	//
	// Defined as constexpr when the targeted language version is higher than or equal to
	// C++20. Defined as nothing otherwise.
	//
	#if !defined(KM_CPP20_CONSTEXPR)
		#if defined(KM_COMPILER_CPP20_ENABLED)
			#define KM_CPP20_CONSTEXPR constexpr
		#else
			#define KM_CPP20_CONSTEXPR
			#define KM_NO_CPP20_CONSTEXPR 1
		#endif
	#endif

	// ------------------------------------------------------------------------
	// KM_EXTERN_TEMPLATE
	// 
	// Portable wrapper for C++11's 'extern template' support.
	//
	// Example usage:
	//     KM_EXTERN_TEMPLATE(class basic_string<char>);
	//
	#if !defined(KM_EXTERN_TEMPLATE)
	#if defined(KM_COMPILER_NO_EXTERN_TEMPLATE)
		#define KM_EXTERN_TEMPLATE(declaration)
	#else
		#define KM_EXTERN_TEMPLATE(declaration) extern template declaration
	#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_NOEXCEPT_IF(predicate)
	// KM_NOEXCEPT_EXPR(expression)
	//
	// Portable wrapper for C++11 noexcept
	// http://en.cppreference.com/w/cpp/language/noexcept
	// http://en.cppreference.com/w/cpp/language/noexcept_spec
	//
	// Example usage:
	//     KM_NOEXCEPT_IF(predicate)
	//     KM_NOEXCEPT_EXPR(expression)
	//
	//     This function never throws an exception.
	//     void DoNothing() noexcept
	//         { }
	//
	//     This function throws an exception of T::T() throws an exception.
	//     template <class T>
	//     void DoNothing() KM_NOEXCEPT_IF(KM_NOEXCEPT_EXPR(T()))
	//         { T t; }
	//
	#if !defined(KM_NOEXCEPT_IF)
			#define KM_NOEXCEPT_IF(predicate) noexcept((predicate))
			#define KM_NOEXCEPT_EXPR(expression) noexcept((expression))
	#endif


	// ------------------------------------------------------------------------
	// KM_CARRIES_DEPENDENCY
	// 
	// Wraps the C++11 carries_dependency attribute
	// http://en.cppreference.com/w/cpp/language/attributes
	// http://blog.aaronballman.com/2011/09/understanding-attributes/
	//
	// Example usage:
	//     KM_CARRIES_DEPENDENCY int* SomeFunction()
	//         { return &mX; }
	// 
	//
	#if !defined(KM_CARRIES_DEPENDENCY)
		#if defined(KM_COMPILER_NO_CARRIES_DEPENDENCY)
			#define KM_CARRIES_DEPENDENCY
		#else
			#define KM_CARRIES_DEPENDENCY [[carries_dependency]]
		#endif
	#endif

	
	// ------------------------------------------------------------------------
	// KM_FALLTHROUGH
	// 
	// [[fallthrough] is a C++17 standard attribute that appears in switch
	// statements to indicate that the fallthrough from the previous case in the
	// switch statement is intentially and not a bug.
	// 
	// http://en.cppreference.com/w/cpp/language/attributes
	//
	// Example usage:
	// 		void f(int n)
	// 		{
	// 			switch(n)
	// 			{
	// 				case 1:
	// 				DoCase1();
	// 				// Compiler may generate a warning for fallthrough behaviour
	// 		 
	// 				case 2: 
	// 				DoCase2();
	//
	// 				KM_FALLTHROUGH;
	// 				case 3:
	// 				DoCase3();
	// 			}
	// 		}
	//
	#if !defined(KM_FALLTHROUGH)
		#if defined(KM_COMPILER_NO_FALLTHROUGH)
			#define KM_FALLTHROUGH
		#else
			#define KM_FALLTHROUGH [[fallthrough]]
		#endif
	#endif

	// KM_MAYBE_UNUSED
	// 
	// [[maybe_unused]] is a C++17 standard attribute that suppresses warnings
	// on unused entities that are declared as maybe_unused.
	//
	// http://en.cppreference.com/w/cpp/language/attributes
	//
	// Example usage:
	//    void foo(KM_MAYBE_UNUSED int i)
	//    {
	//        assert(i == 42);  // warning suppressed when asserts disabled.
	//    }
	//
	#if !defined(KM_MAYBE_UNUSED)
		#if defined(KM_COMPILER_NO_MAYBE_UNUSED)
			#define KM_MAYBE_UNUSED
		#else
			#define KM_MAYBE_UNUSED [[maybe_unused]]
		#endif
	#endif

	
	// ------------------------------------------------------------------------
	// KM_NO_UBSAN
	// 
	// The LLVM/Clang undefined behaviour sanitizer will not analyse a function tagged with the following attribute.
	//
	// https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#disabling-instrumentation-with-attribute-no-sanitize-undefined
	//
	// Example usage:
	//     KM_NO_UBSAN int SomeFunction() { ... }
	//
	#ifndef KM_NO_UBSAN
		#if defined(KM_COMPILER_CLANG)
			#define KM_NO_UBSAN __attribute__((no_sanitize("undefined")))
		#else
			#define KM_NO_UBSAN
		#endif
	#endif
	

	// ------------------------------------------------------------------------
	// KM_NO_ASAN
	// 
	// The LLVM/Clang address sanitizer will not analyse a function tagged with the following attribute.
	//
	// https://clang.llvm.org/docs/AddressSanitizer.html#disabling-instrumentation-with-attribute-no-sanitize-address
	//
	// Example usage:
	//     KM_NO_ASAN int SomeFunction() { ... }
	//
	#ifndef KM_NO_ASAN
		#if defined(KM_COMPILER_CLANG)
			#define KM_NO_ASAN __attribute__((no_sanitize("address")))
		#else
			#define KM_NO_ASAN
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_ASAN_ENABLED
	//
	// Defined as 0 or 1. It's value depends on the compile environment.
	// Specifies whether the code is being built with Clang's Address Sanitizer.
	//
	#if defined(__has_feature)
		#if __has_feature(address_sanitizer)
			#define KM_ASAN_ENABLED 1
		#else
			#define KM_ASAN_ENABLED 0
		#endif
	#else
		#define KM_ASAN_ENABLED 0
	#endif

	/////////////////////////////////////////////////////////////////////////////////
	//
	// KM_UBSAN_ENABLED
	//
	// Defined as 0 or 1. It's value depends on the compile environment.
	// Specifies whether the code is being built with Undefined Behavior Sanitizer.
	//
	#if KM_COMPILER_HAS_FEATURE(undefined_behavior_sanitizer)
		#define KM_UBSAN_ENABLED 1
	#else
		#define KM_UBSAN_ENABLED 0
	#endif

	/////////////////////////////////////////////////////////////////////////////////
	//
	// KM_MSAN_ENABLED
	//
	// Defined as 0 or 1. It's value depends on the compile environment.
	// Specifies whether the code is being built with Memory Sanitizer.
	//
	// MSAN documentation:
	// https://clang.llvm.org/docs/MemorySanitizer.html
	// https://github.com/google/sanitizers/wiki/MemorySanitizer
	//
	#if KM_COMPILER_HAS_FEATURE(memory_sanitizer)
		#define KM_MSAN_ENABLED 1
	#else
		#define KM_MSAN_ENABLED 0
	#endif

	// ------------------------------------------------------------------------
	// KM_NON_COPYABLE
	//
	// This macro defines as a class as not being copy-constructable
	// or assignable. This is useful for preventing class instances 
	// from being passed to functions by value, is useful for preventing
	// compiler warnings by some compilers about the inability to 
	// auto-generate a copy constructor and assignment, and is useful 
	// for simply declaring in the interface that copy semantics are
	// not supported by the class. Your class needs to have at least a
	// default constructor when using this macro.
	//
	// Beware that this class works by declaring a private: section of 
	// the class in the case of compilers that don't support C++11 deleted
	// functions. 
	//
	// Note: With some pre-C++11 compilers (e.g. Green Hills), you may need 
	//       to manually define an instances of the hidden functions, even 
	//       though they are not used.
	//
	// Example usage:
	//    class Widget {
	//       Widget();
	//       . . .
	//       KM_NON_COPYABLE(Widget)
	//    };
	//
	#if !defined(KM_NON_COPYABLE)
		#if defined(KM_COMPILER_NO_DELETED_FUNCTIONS)
			#define KM_NON_COPYABLE(EAClass_)               \
			  private:                                      \
				KM_DISABLE_VC_WARNING(4822);	/* local class member function does not have a body	*/		\
				EAClass_(const EAClass_&);                  \
				void operator=(const EAClass_&);			\
				KM_RESTORE_VC_WARNING();
		#else
			#define KM_NON_COPYABLE(EAClass_)               \
				KM_DISABLE_VC_WARNING(4822);	/* local class member function does not have a body	*/		\
				EAClass_(const EAClass_&) = delete;         \
				void operator=(const EAClass_&) = delete;	\
				KM_RESTORE_VC_WARNING();
		#endif
	#endif


	// ------------------------------------------------------------------------
	// KM_FUNCTION_DELETE
	//
	// Semi-portable way of specifying a deleted function which allows for 
	// cleaner code in class declarations. 
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private: // For portability with pre-C++11 compilers, make the function private.
	//      void foo() KM_FUNCTION_DELETE;
	//  };
	//
	// Note: KM_FUNCTION_DELETE'd functions should be private to prevent the
	// functions from being called even when the compiler does not support 
	// deleted functions. Some compilers (e.g. Green Hills) that don't support 
	// C++11 deleted functions can require that you define the function,
	// which you can do in the associated source file for the class.
	//
	#if defined(KM_COMPILER_NO_DELETED_FUNCTIONS)
		#define KM_FUNCTION_DELETE
	#else
		#define KM_FUNCTION_DELETE = delete
	#endif

	// ------------------------------------------------------------------------
	// KM_DISABLE_DEFAULT_CTOR
	//
	// Disables the compiler generated default constructor. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      KM_DISABLE_DEFAULT_CTOR(Example);
	//  };
	//
	#define KM_DISABLE_DEFAULT_CTOR(ClassName) ClassName() KM_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// KM_DISABLE_COPY_CTOR
	//
	// Disables the compiler generated copy constructor. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      KM_DISABLE_COPY_CTOR(Example);
	//  };
	//
	#define KM_DISABLE_COPY_CTOR(ClassName) ClassName(const ClassName &) KM_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// KM_DISABLE_MOVE_CTOR
	//
	// Disables the compiler generated move constructor. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      KM_DISABLE_MOVE_CTOR(Example);
	//  };
	//
	#define KM_DISABLE_MOVE_CTOR(ClassName) ClassName(ClassName&&) KM_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// KM_DISABLE_ASSIGNMENT_OPERATOR
	//
	// Disables the compiler generated assignment operator. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      KM_DISABLE_ASSIGNMENT_OPERATOR(Example);
	//  };
	//
	#define KM_DISABLE_ASSIGNMENT_OPERATOR(ClassName) ClassName & operator=(const ClassName &) KM_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// KM_DISABLE_MOVE_OPERATOR
	//
	// Disables the compiler generated move operator. This macro is
	// provided to improve portability and clarify intent of code.
	//
	// Example usage:
	//
	//  class Example
	//  {
	//  private:
	//      KM_DISABLE_MOVE_OPERATOR(Example);
	//  };
	//
	#define KM_DISABLE_MOVE_OPERATOR(ClassName) ClassName & operator=(ClassName&&) KM_FUNCTION_DELETE

	// ------------------------------------------------------------------------
	// EANonCopyable
	//
	// Declares a class as not supporting copy construction or assignment.
	// May be more reliable with some situations that KM_NON_COPYABLE alone,
	// though it may result in more code generation.
	//
	// Note that VC++ will generate warning C4625 and C4626 if you use EANonCopyable
	// and you are compiling with /W4 and /Wall. There is no resolution but
	// to redelare KM_NON_COPYABLE in your subclass or disable the warnings with
	// code like this:
	//     KM_DISABLE_VC_WARNING(4625 4626)
	//     ...
	//     KM_RESTORE_VC_WARNING()
	//
	// Example usage:
	//     struct Widget : EANonCopyable {
	//        . . .
	//     };
	//
	#ifdef __cplusplus
		struct EANonCopyable
		{
			#if defined(KM_COMPILER_NO_DEFAULTED_FUNCTIONS) ||  defined(__EDG__)
				// EDG doesn't appear to behave properly for the case of defaulted constructors; 
				// it generates a mistaken warning about missing default constructors.					 
				EANonCopyable() {}  // Putting {} here has the downside that it allows a class to create itself,
				~EANonCopyable() {} // but avoids linker errors that can occur with some compilers (e.g. Green Hills).
			#else
				EANonCopyable() = default;
			   ~EANonCopyable() = default;
			#endif

			KM_NON_COPYABLE(EANonCopyable)
		};
	#endif


	// ------------------------------------------------------------------------
	// KM_OPTIMIZE_OFF / KM_OPTIMIZE_ON
	//
	// Implements portable inline optimization enabling/disabling.
	// Usage of these macros must be in order OFF then ON. This is 
	// because the OFF macro pushes a set of settings and the ON
	// macro pops them. The nesting of OFF/ON sets (e.g. OFF, OFF, ON, ON)
	// is not guaranteed to work on all platforms.
	//
	// This is often used to allow debugging of some code that's 
	// otherwise compiled with undebuggable optimizations. It's also
	// useful for working around compiler code generation problems
	// that occur in optimized builds.
	//
	// Some compilers (e.g. VC++) don't allow doing this within a function and 
	// so the usage must be outside a function, as with the example below.
	// GCC on x86 appears to have some problem with argument passing when 
	// using KM_OPTIMIZE_OFF in optimized builds.
	//
	// Example usage:
	//     // Disable optimizations for SomeFunction.
	//     KM_OPTIMIZE_OFF()
	//     void SomeFunction()
	//     {
	//         ...
	//     }
	//     KM_OPTIMIZE_ON()
	//
	#if !defined(KM_OPTIMIZE_OFF)
		#if   defined(KM_COMPILER_MSVC)
			#define KM_OPTIMIZE_OFF() __pragma(optimize("", off))
		#elif defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION > 4004) && (defined(__i386__) || defined(__x86_64__)) // GCC 4.4+ - Seems to work only on x86/Linux so far. However, GCC 4.4 itself appears broken and screws up parameter passing conventions.
			#define KM_OPTIMIZE_OFF()            \
				_Pragma("GCC push_options")      \
				_Pragma("GCC optimize 0")
        #elif defined(KM_COMPILER_CLANG) && (!defined(KM_PLATFORM_ANDROID) || (KM_COMPILER_VERSION >= 380))
            #define KM_OPTIMIZE_OFF() \
				KM_DISABLE_CLANG_WARNING(-Wunknown-pragmas) \
				_Pragma("clang optimize off") \
				KM_RESTORE_CLANG_WARNING()
		#else
			#define KM_OPTIMIZE_OFF()
		#endif
	#endif

	#if !defined(KM_OPTIMIZE_ON)
		#if   defined(KM_COMPILER_MSVC)
			#define KM_OPTIMIZE_ON() __pragma(optimize("", on))
		#elif defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION > 4004) && (defined(__i386__) || defined(__x86_64__)) // GCC 4.4+ - Seems to work only on x86/Linux so far. However, GCC 4.4 itself appears broken and screws up parameter passing conventions.
			#define KM_OPTIMIZE_ON() _Pragma("GCC pop_options")
        #elif defined(KM_COMPILER_CLANG) && (!defined(KM_PLATFORM_ANDROID) || (KM_COMPILER_VERSION >= 380))
            #define KM_OPTIMIZE_ON() \
				KM_DISABLE_CLANG_WARNING(-Wunknown-pragmas) \
				_Pragma("clang optimize on") \
				KM_RESTORE_CLANG_WARNING()
		#else
			#define KM_OPTIMIZE_ON()
		#endif
	#endif



	// ------------------------------------------------------------------------
	// KM_SIGNED_RIGHT_SHIFT_IS_UNSIGNED
	//
	// Defined if right shifts of signed integers (i.e. arithmetic shifts) fail 
	// to propogate the high bit downward, and thus preserve sign. Most hardware 
	// and their corresponding compilers do this.
	//
	// <No current platform fails to propogate sign bits on right signed shifts>

#endif // Header include guard










