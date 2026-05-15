/*-----------------------------------------------------------------------------
 * config/eaplatform.h
 *
 * Copyright (c) Electronic Arts Inc. All rights reserved.
 *-----------------------------------------------------------------------------
 * Currently supported platform indentification defines include:
 */
#ifdef KM_PLATFORM_PS4 // ifdef for code stripping purposes
// KM_PLATFORM_PS4 (KM_PLATFORM_KETTLE)
#endif
#ifdef KM_PLATFORM_XBOXONE // ifdef for code stripping purposes
 // KM_PLATFORM_XBOXONE (KM_PLATFORM_CAPILANO)
 // KM_PLATFORM_XBOXONE_XDK (KM_PLATFORM_CAPILANO_XDK), set by capilano_config package
 // KM_PLATFORM_XBOXONE_ADK (KM_PLATFORM_CAPILANO_ADK), set by capilano_config package
#endif
//    KM_PLATFORM_ANDROID
//    KM_PLATFORM_APPLE
//    KM_PLATFORM_IPHONE
//    KM_PLATFORM_IPHONE_SIMULATOR
//    KM_PLATFORM_OSX
//    KM_PLATFORM_LINUX
//    KM_PLATFORM_SAMSUNG_TV
//    KM_PLATFORM_WINDOWS
//    KM_PLATFORM_WIN32
//    KM_PLATFORM_WIN64
//    KM_PLATFORM_WINDOWS_PHONE
//    KM_PLATFORM_WINRT
//    KM_PLATFORM_SUN
//    KM_PLATFORM_LRB (Larrabee)
//    KM_PLATFORM_POSIX     (pseudo-platform; may be defined along with another platform like KM_PLATFORM_LINUX, KM_PLATFORM_UNIX, KM_PLATFORM_QNX)
//    KM_PLATFORM_UNIX      (pseudo-platform; may be defined along with another platform like KM_PLATFORM_LINUX)
//    KM_PLATFORM_CYGWIN    (pseudo-platform; may be defined along with another platform like KM_PLATFORM_LINUX)
//    KM_PLATFORM_MINGW     (pseudo-platform; may be defined along with another platform like KM_PLATFORM_WINDOWS)
//    KM_PLATFORM_MICROSOFT (pseudo-platform; may be defined along with another platform like KM_PLATFORM_WINDOWS)
//
//    KM_ABI_ARM_LINUX      (a.k.a. "eabi". for all platforms that use the CodeSourcery GNU/Linux toolchain, like Android)
//    KM_ABI_ARM_APPLE      (similar to eabi but not identical)
//    KM_ABI_ARM64_APPLE    (similar to eabi but not identical) https://developer.apple.com/library/ios/documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARM64FunctionCallingConventions.html
//    KM_ABI_ARM_WINCE      (similar to eabi but not identical)
//
// Other definitions emanated from this file inclue:
//    KM_PLATFORM_NAME = <string>
//    KM_PLATFORM_DESCRIPTION = <string>
//    KM_PROCESSOR_XXX
//    KM_MISALIGNED_SUPPORT_LEVEL=0|1|2
//    KM_SYSTEM_LITTLE_ENDIAN | KM_SYSTEM_BIG_ENDIAN
//    KM_ASM_STYLE_ATT | KM_ASM_STYLE_INTEL | KM_ASM_STYLE_MOTOROLA
//    KM_PLATFORM_PTR_SIZE = <integer size in bytes>
//    KM_PLATFORM_WORD_SIZE = <integer size in bytes>
//    KM_CACHE_LINE_SIZE = <integer size in bytes>
//---------------------------------------------------------------------------

/*
	KM_PLATFORM_MOBILE
	KM_PLATFORM_MOBILE is a peer to KM_PLATORM_DESKTOP and KM_PLATFORM_CONSOLE. Their definition is qualitative rather
	than quantitative, and refers to the general (usually weaker) capabilities of the machine. Mobile devices have a
	similar set of weaknesses that are useful to generally categorize. The primary motivation is to avoid code that
	tests for multiple mobile platforms on a line and needs to be updated every time we get a new one.
	For example, mobile platforms tend to have weaker ARM processors, don't have full multiple processor support,
	are hand-held, don't have mice (though may have touch screens or basic cursor controls), have writable solid
	state permanent storage. Production user code shouldn't have too many expectations about the meaning of this define.

	KM_PLATFORM_DESKTOP
	This is similar to KM_PLATFORM_MOBILE in its qualitative nature and refers to platforms that are powerful.
	For example, they nearly always have virtual memory, mapped memory, hundreds of GB of writable disk storage,
	TCP/IP network connections, mice, keyboards, 512+ MB of RAM, multiprocessing, multiple display support.
	Production user code shouldn't have too many expectations about the meaning of this define.

	KM_PLATFORM_CONSOLE
	This is similar to KM_PLATFORM_MOBILE in its qualitative nature and refers to platforms that are consoles.
	This means platforms that are connected to TVs, are fairly powerful (especially graphics-wise), are tightly
	controlled by vendors, tend not to have mapped memory, tend to have TCP/IP, don't have multiple process support
	though they might have multiple CPUs, support TV output only. Production user code shouldn't have too many
	expectations about the meaning of this define.

*/


#ifndef INCLUDED_eaplatform_H
#define INCLUDED_eaplatform_H


// Cygwin
// This is a pseudo-platform which will be defined along with KM_PLATFORM_LINUX when
// using the Cygwin build environment.
#if defined(__CYGWIN__)
	#define KM_PLATFORM_CYGWIN 1
	#define KM_PLATFORM_DESKTOP 1
#endif

// MinGW
// This is a pseudo-platform which will be defined along with KM_PLATFORM_WINDOWS when
// using the MinGW Windows build environment.
#if defined(__MINGW32__) || defined(__MINGW64__)
	#define KM_PLATFORM_MINGW 1
	#define KM_PLATFORM_DESKTOP 1
#endif

#if defined(KM_PLATFORM_PS4) || defined(__ORBIS__) || defined(KM_PLATFORM_KETTLE)
	// PlayStation 4
	// Orbis was Sony's code-name for the platform, which is now obsolete.
	// Kettle was an EA-specific code-name for the platform, which is now obsolete.
	#if defined(KM_PLATFORM_PS4)
		#undef  KM_PLATFORM_PS4
	#endif
	#define KM_PLATFORM_PS4 1

	// Backward compatibility:
		#if defined(KM_PLATFORM_KETTLE)
			#undef  KM_PLATFORM_KETTLE
		#endif
	// End backward compatbility

	#define KM_PLATFORM_KETTLE 1
	#define KM_PLATFORM_NAME "PS4"
	#define KM_SYSTEM_LITTLE_ENDIAN 1
	#define KM_PLATFORM_DESCRIPTION "PS4 on x64"
	#define KM_PLATFORM_CONSOLE 1
	#define KM_PLATFORM_SONY 1
	#define KM_PLATFORM_POSIX 1
	// #define KM_POSIX_THREADS_AVAILABLE 1  // POSIX threading API is available but discouraged.  Sony indicated use of the scePthreads* API is preferred.
	#define KM_PROCESSOR_X86_64 1
	#if defined(__GNUC__) || defined(__clang__)
		#define KM_ASM_STYLE_ATT 1
	#endif

#elif defined(KM_PLATFORM_XBOXONE) || defined(_DURANGO) || defined(_XBOX_ONE) || defined(KM_PLATFORM_CAPILANO) || defined(_GAMING_XBOX)
	// XBox One
	// Durango was Microsoft's code-name for the platform, which is now obsolete.
	// Microsoft uses _DURANGO instead of some variation of _XBOX, though it's not natively defined by the compiler.
	// Capilano was an EA-specific code-name for the platform, which is now obsolete.
	#if defined(KM_PLATFORM_XBOXONE)
		#undef  KM_PLATFORM_XBOXONE
	#endif
	#define KM_PLATFORM_XBOXONE 1

	// Backward compatibility:
		#if defined(KM_PLATFORM_CAPILANO)
			#undef  KM_PLATFORM_CAPILANO
		#endif
		#define KM_PLATFORM_CAPILANO 1
		#if defined(KM_PLATFORM_CAPILANO_XDK) && !defined(KM_PLATFORM_XBOXONE_XDK)
			#define KM_PLATFORM_XBOXONE_XDK 1
		#endif
		#if defined(KM_PLATFORM_CAPILANO_ADK) && !defined(KM_PLATFORM_XBOXONE_ADK)
			#define KM_PLATFORM_XBOXONE_ADK 1
		#endif
	// End backward compatibility

	#if !defined(_DURANGO)
		#define _DURANGO
	#endif
	#define KM_PLATFORM_NAME "XBox One"
  //#define KM_PROCESSOR_X86  Currently our policy is that we don't define this, even though x64 is something of a superset of x86.
	#define KM_PROCESSOR_X86_64 1
	#define KM_SYSTEM_LITTLE_ENDIAN 1
	#define KM_PLATFORM_DESCRIPTION "XBox One on x64"
	#define KM_ASM_STYLE_INTEL 1
	#define KM_PLATFORM_CONSOLE 1
	#define KM_PLATFORM_MICROSOFT 1

	// WINAPI_FAMILY defines - mirrored from winapifamily.h
	#define KM_WINAPI_FAMILY_APP         1000
	#define KM_WINAPI_FAMILY_DESKTOP_APP 1001
	#define KM_WINAPI_FAMILY_PHONE_APP   1002
	#define KM_WINAPI_FAMILY_TV_APP      1003
	#define KM_WINAPI_FAMILY_TV_TITLE    1004
	#define KM_WINAPI_FAMILY_GAMES       1006
	
	#if defined(WINAPI_FAMILY) 
		#include <winapifamily.h>
		#if defined(WINAPI_FAMILY_TV_TITLE) && WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
			#define KM_WINAPI_FAMILY KM_WINAPI_FAMILY_TV_TITLE
		#elif defined(WINAPI_FAMILY_DESKTOP_APP) && WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
			#define KM_WINAPI_FAMILY KM_WINAPI_FAMILY_DESKTOP_APP
		#elif defined(WINAPI_FAMILY_GAMES) && WINAPI_FAMILY == WINAPI_FAMILY_GAMES
			#define KM_WINAPI_FAMILY KM_WINAPI_FAMILY_GAMES
		#else
			#error Unsupported WINAPI_FAMILY
		#endif
	#else
		#error WINAPI_FAMILY should always be defined on Capilano.
	#endif

	// Macro to determine if a partition is enabled.
	#define KM_WINAPI_FAMILY_PARTITION(Partition)	(Partition)

	#if KM_WINAPI_FAMILY == KM_WINAPI_FAMILY_DESKTOP_APP
		#define KM_WINAPI_PARTITION_CORE     1
		#define KM_WINAPI_PARTITION_DESKTOP  1
		#define KM_WINAPI_PARTITION_APP      1
		#define KM_WINAPI_PARTITION_PC_APP   0
		#define KM_WIANPI_PARTITION_PHONE    0
		#define KM_WINAPI_PARTITION_TV_APP   0
		#define KM_WINAPI_PARTITION_TV_TITLE 0
		#define KM_WINAPI_PARTITION_GAMES    0
	#elif KM_WINAPI_FAMILY == KM_WINAPI_FAMILY_TV_TITLE
		#define KM_WINAPI_PARTITION_CORE     1
		#define KM_WINAPI_PARTITION_DESKTOP  0
		#define KM_WINAPI_PARTITION_APP      0
		#define KM_WINAPI_PARTITION_PC_APP   0
		#define KM_WIANPI_PARTITION_PHONE    0
		#define KM_WINAPI_PARTITION_TV_APP   0
		#define KM_WINAPI_PARTITION_TV_TITLE 1
		#define KM_WINAPI_PARTITION_GAMES    0
	#elif KM_WINAPI_FAMILY == KM_WINAPI_FAMILY_GAMES
		#define KM_WINAPI_PARTITION_CORE     1
		#define KM_WINAPI_PARTITION_DESKTOP  0
		#define KM_WINAPI_PARTITION_APP      0
		#define KM_WINAPI_PARTITION_PC_APP   0
		#define KM_WIANPI_PARTITION_PHONE    0
		#define KM_WINAPI_PARTITION_TV_APP   0
		#define KM_WINAPI_PARTITION_TV_TITLE 0
		#define KM_WINAPI_PARTITION_GAMES    1
	#else
		#error Unsupported WINAPI_FAMILY
	#endif

	#if KM_WINAPI_FAMILY_PARTITION(KM_WINAPI_PARTITION_GAMES)
		#define CS_UNDEFINED_STRING 			1
		#define CS_UNDEFINED_STRING 		1
	#endif

	#if KM_WINAPI_FAMILY_PARTITION(KM_WINAPI_PARTITION_TV_TITLE)
		#define KM_PLATFORM_XBOXONE_XDK 	1
	#endif
#elif defined(KM_PLATFORM_LRB) || defined(__LRB__) || (defined(__EDG__) && defined(__ICC) && defined(__x86_64__))
	#undef  KM_PLATFORM_LRB
	#define KM_PLATFORM_LRB         1
	#define KM_PLATFORM_NAME        "Larrabee"
	#define KM_PLATFORM_DESCRIPTION "Larrabee on LRB1"
	#define KM_PROCESSOR_X86_64 1
	#if defined(BYTE_ORDER) && (BYTE_ORDER == 4321)
		#define KM_SYSTEM_BIG_ENDIAN 1
	#else
		#define KM_SYSTEM_LITTLE_ENDIAN 1
	#endif
	#define KM_PROCESSOR_LRB 1
	#define KM_PROCESSOR_LRB1 1       // Larrabee version 1
	#define KM_ASM_STYLE_ATT 1        // Both types of asm style
	#define KM_ASM_STYLE_INTEL 1      // are supported.
	#define KM_PLATFORM_DESKTOP 1

// Android (Google phone OS)
#elif defined(KM_PLATFORM_ANDROID) || defined(__ANDROID__)
	#undef  KM_PLATFORM_ANDROID
	#define KM_PLATFORM_ANDROID 1
	#define KM_PLATFORM_LINUX 1
	#define KM_PLATFORM_UNIX 1
	#define KM_PLATFORM_POSIX 1
	#define KM_PLATFORM_NAME "Android"
	#define KM_ASM_STYLE_ATT 1
	#if defined(__arm__)
		#define KM_ABI_ARM_LINUX 1  // a.k.a. "ARM eabi"
		#define KM_PROCESSOR_ARM32 1
		#define KM_PLATFORM_DESCRIPTION "Android on ARM"
	#elif defined(__aarch64__)
		#define KM_PROCESSOR_ARM64 1
		#define KM_PLATFORM_DESCRIPTION "Android on ARM64"
	#elif defined(__i386__)
		#define KM_PROCESSOR_X86 1
		#define KM_PLATFORM_DESCRIPTION "Android on x86"
	#elif defined(__x86_64)
		#define KM_PROCESSOR_X86_64 1
		#define KM_PLATFORM_DESCRIPTION "Android on x64"
	#else
		#error Unknown processor
	#endif
	#if !defined(KM_SYSTEM_BIG_ENDIAN) && !defined(KM_SYSTEM_LITTLE_ENDIAN)
		#define KM_SYSTEM_LITTLE_ENDIAN 1
	#endif
	#define KM_PLATFORM_MOBILE 1

// Samsung SMART TV - a Linux-based smart TV
#elif defined(KM_PLATFORM_SAMSUNG_TV)
	#undef  KM_PLATFORM_SAMSUNG_TV
	#define KM_PLATFORM_SAMSUNG_TV 1
	#define KM_PLATFORM_LINUX 1
	#define KM_PLATFORM_UNIX 1
	#define KM_PLATFORM_POSIX 1
	#define KM_PLATFORM_NAME "SamsungTV"
	#define KM_PLATFORM_DESCRIPTION "Samsung SMART TV on ARM"
	#define KM_ASM_STYLE_ATT 1
	#define KM_SYSTEM_LITTLE_ENDIAN 1
	#define KM_PROCESSOR_ARM32 1
	#define KM_ABI_ARM_LINUX 1 // a.k.a. "ARM eabi"
	#define KM_PROCESSOR_ARM7 1

#elif defined(__APPLE__) && __APPLE__
	#include <TargetConditionals.h>

	// Apple family of operating systems.
	#define KM_PLATFORM_APPLE
	#define KM_PLATFORM_POSIX 1

	// iPhone
	// TARGET_OS_IPHONE will be undefined on an unknown compiler, and will be defined on gcc.
	#if defined(KM_PLATFORM_IPHONE) || defined(__IPHONE__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR)
		#undef  KM_PLATFORM_IPHONE
		#define KM_PLATFORM_IPHONE 1
		#define KM_PLATFORM_NAME "iPhone"
		#define KM_ASM_STYLE_ATT 1
		#define KM_POSIX_THREADS_AVAILABLE 1
		#if defined(__arm__)
			#define KM_ABI_ARM_APPLE 1
			#define KM_PROCESSOR_ARM32 1
			#define KM_SYSTEM_LITTLE_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "iPhone on ARM"
		#elif defined(__aarch64__) || defined(__AARCH64)
			#define KM_ABI_ARM64_APPLE 1
			#define KM_PROCESSOR_ARM64 1
			#define KM_SYSTEM_LITTLE_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "iPhone on ARM64"
		#elif defined(__i386__)
			#define KM_PLATFORM_IPHONE_SIMULATOR 1
			#define KM_PROCESSOR_X86 1
			#define KM_SYSTEM_LITTLE_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "iPhone simulator on x86"
		#elif defined(__x86_64) || defined(__amd64)
			#define KM_PROCESSOR_X86_64 1
			#define KM_SYSTEM_LITTLE_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "iPhone simulator on x64"
		#else
			#error Unknown processor
		#endif
		#define KM_PLATFORM_MOBILE 1

	// Macintosh OSX
	// TARGET_OS_MAC is defined by the Metrowerks and older AppleC compilers.
	// Howerver, TARGET_OS_MAC is defined to be 1 in all cases.
	// __i386__ and __intel__ are defined by the GCC compiler.
	// __dest_os is defined by the Metrowerks compiler.
	// __MACH__ is defined by the Metrowerks and GCC compilers.
	// powerc and __powerc are defined by the Metrowerks and GCC compilers.
	#elif defined(KM_PLATFORM_OSX) || defined(__MACH__) || (defined(__MSL__) && (__dest_os == __mac_os_x))
		#undef  KM_PLATFORM_OSX
		#define KM_PLATFORM_OSX 1
		#define KM_PLATFORM_UNIX 1
		#define KM_PLATFORM_POSIX 1
	  //#define KM_PLATFORM_BSD 1           We don't currently define this. OSX has some BSD history but a lot of the API is different.
		#define KM_PLATFORM_NAME "OSX"
		#if defined(__i386__) || defined(__intel__)
			#define KM_PROCESSOR_X86 1
			#define KM_SYSTEM_LITTLE_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "OSX on x86"
		#elif defined(__x86_64) || defined(__amd64)
			#define KM_PROCESSOR_X86_64 1
			#define KM_SYSTEM_LITTLE_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "OSX on x64"
		#elif defined(__arm__)
			#define KM_ABI_ARM_APPLE 1
			#define KM_PROCESSOR_ARM32 1
			#define KM_SYSTEM_LITTLE_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "OSX on ARM"
		#elif defined(__aarch64__) || defined(__AARCH64)
			#define KM_ABI_ARM64_APPLE 1
			#define KM_PROCESSOR_ARM64 1
			#define KM_SYSTEM_LITTLE_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "OSX on ARM64"
		#elif defined(__POWERPC64__) || defined(__powerpc64__)
			#define KM_PROCESSOR_POWERPC 1
			#define KM_PROCESSOR_POWERPC_64 1
			#define KM_SYSTEM_BIG_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "OSX on PowerPC 64"
		#elif defined(__POWERPC__) || defined(__powerpc__)
			#define KM_PROCESSOR_POWERPC 1
			#define KM_PROCESSOR_POWERPC_32 1
			#define KM_SYSTEM_BIG_ENDIAN 1
			#define KM_PLATFORM_DESCRIPTION "OSX on PowerPC"
		#else
			#error Unknown processor
		#endif
		#if defined(__GNUC__)
			#define KM_ASM_STYLE_ATT 1
		#else
			#define KM_ASM_STYLE_MOTOROLA 1
		#endif
		#define KM_PLATFORM_DESKTOP 1
	#else
		#error Unknown Apple Platform
	#endif

// Linux
// __linux and __linux__ are defined by the GCC and Borland compiler.
// __i386__ and __intel__ are defined by the GCC compiler.
// __i386__ is defined by the Metrowerks compiler.
// _M_IX86 is defined by the Borland compiler.
// __sparc__ is defined by the GCC compiler.
// __powerpc__ is defined by the GCC compiler.
// __ARM_EABI__ is defined by GCC on an ARM v6l (Raspberry Pi 1)
// __ARM_ARCH_7A__ is defined by GCC on an ARM v7l (Raspberry Pi 2)
#elif defined(KM_PLATFORM_LINUX) || (defined(__linux) || defined(__linux__))
	#undef  KM_PLATFORM_LINUX
	#define KM_PLATFORM_LINUX 1
	#define KM_PLATFORM_UNIX 1
	#define KM_PLATFORM_POSIX 1
	#define KM_PLATFORM_NAME "Linux"
	#if defined(__i386__) || defined(__intel__) || defined(_M_IX86)
		#define KM_PROCESSOR_X86 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Linux on x86"
	#elif defined(__ARM_ARCH_7A__) || defined(__ARM_EABI__)
		#define KM_ABI_ARM_LINUX 1
		#define KM_PROCESSOR_ARM32 1
		#define KM_PLATFORM_DESCRIPTION "Linux on ARM 6/7 32-bits"
	#elif defined(__aarch64__) || defined(__AARCH64)
		#define KM_PROCESSOR_ARM64 1
		#define KM_PLATFORM_DESCRIPTION "Linux on ARM64"
	#elif defined(__x86_64__)
		#define KM_PROCESSOR_X86_64 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Linux on x64"
	#elif defined(__powerpc64__)
		#define KM_PROCESSOR_POWERPC 1
		#define KM_PROCESSOR_POWERPC_64 1
		#define KM_SYSTEM_BIG_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Linux on PowerPC 64"
	#elif defined(__powerpc__)
		#define KM_PROCESSOR_POWERPC 1
		#define KM_PROCESSOR_POWERPC_32 1
		#define KM_SYSTEM_BIG_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Linux on PowerPC"
	#else
		#error Unknown processor
		#error Unknown endianness
	#endif
	#if defined(__GNUC__)
		#define KM_ASM_STYLE_ATT 1
	#endif
	#define KM_PLATFORM_DESKTOP 1


#elif defined(KM_PLATFORM_BSD) || (defined(__BSD__) || defined(__FreeBSD__))
	#undef  KM_PLATFORM_BSD
	#define KM_PLATFORM_BSD 1
	#define KM_PLATFORM_UNIX 1
	#define KM_PLATFORM_POSIX 1     // BSD's posix complaince is not identical to Linux's
	#define KM_PLATFORM_NAME "BSD Unix"
	#if defined(__i386__) || defined(__intel__)
		#define KM_PROCESSOR_X86 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "BSD on x86"
	#elif defined(__x86_64__)
		#define KM_PROCESSOR_X86_64 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "BSD on x64"
	#elif defined(__powerpc64__)
		#define KM_PROCESSOR_POWERPC 1
		#define KM_PROCESSOR_POWERPC_64 1
		#define KM_SYSTEM_BIG_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "BSD on PowerPC 64"
	#elif defined(__powerpc__)
		#define KM_PROCESSOR_POWERPC 1
		#define KM_PROCESSOR_POWERPC_32 1
		#define KM_SYSTEM_BIG_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "BSD on PowerPC"
	#else
		#error Unknown processor
		#error Unknown endianness
	#endif
	#if !defined(KM_PLATFORM_FREEBSD) && defined(__FreeBSD__)
		#define KM_PLATFORM_FREEBSD 1 // This is a variation of BSD.
	#endif
	#if defined(__GNUC__)
		#define KM_ASM_STYLE_ATT 1
	#endif
	#define KM_PLATFORM_DESKTOP 1


#elif defined(KM_PLATFORM_WINDOWS_PHONE)
	#undef KM_PLATFORM_WINDOWS_PHONE
	#define KM_PLATFORM_WINDOWS_PHONE 1
	#define KM_PLATFORM_NAME "Windows Phone"
	#if defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
		#define KM_PROCESSOR_X86_64 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Windows Phone on x64"
	#elif defined(_M_IX86) || defined(_X86_)
		#define KM_PROCESSOR_X86 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Windows Phone on X86"
	#elif defined(_M_ARM)
		#define KM_ABI_ARM_WINCE 1
		#define KM_PROCESSOR_ARM32 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Windows Phone on ARM"
	#else //Possibly other Windows Phone variants
		#error Unknown processor
		#error Unknown endianness
	#endif
	#define KM_PLATFORM_MICROSOFT 1

	// WINAPI_FAMILY defines - mirrored from winapifamily.h
	#define KM_WINAPI_FAMILY_APP         1
	#define KM_WINAPI_FAMILY_DESKTOP_APP 2
	#define KM_WINAPI_FAMILY_PHONE_APP   3

	#if defined(WINAPI_FAMILY)
		#include <winapifamily.h>
		#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
			#define KM_WINAPI_FAMILY KM_WINAPI_FAMILY_PHONE_APP
		#else
			#error Unsupported WINAPI_FAMILY for Windows Phone
		#endif
	#else
		#error WINAPI_FAMILY should always be defined on Windows Phone.
	#endif

	// Macro to determine if a partition is enabled.
	#define KM_WINAPI_FAMILY_PARTITION(Partition)   (Partition)

	// Enable the appropriate partitions for the current family
	#if KM_WINAPI_FAMILY == KM_WINAPI_FAMILY_PHONE_APP
	#   define KM_WINAPI_PARTITION_CORE    1
	#   define KM_WINAPI_PARTITION_PHONE   1
	#   define KM_WINAPI_PARTITION_APP     1
	#else
	#   error Unsupported WINAPI_FAMILY for Windows Phone
	#endif


// Windows
// _WIN32 is defined by the VC++, Intel and GCC compilers.
// _WIN64 is defined by the VC++, Intel and GCC compilers.
// __WIN32__ is defined by the Borland compiler.
// __INTEL__ is defined by the Metrowerks compiler.
// _M_IX86, _M_AMD64 and _M_IA64 are defined by the VC++, Intel, and Borland compilers.
// _X86_, _AMD64_, and _IA64_ are defined by the Metrowerks compiler.
// _M_ARM is defined by the VC++ compiler.
#elif (defined(KM_PLATFORM_WINDOWS) || (defined(_WIN32) || defined(__WIN32__) || defined(_WIN64))) && !defined(CS_UNDEFINED_STRING)
	#undef  KM_PLATFORM_WINDOWS
	#define KM_PLATFORM_WINDOWS 1
	#define KM_PLATFORM_NAME "Windows"
	#ifdef _WIN64 // VC++ defines both _WIN32 and _WIN64 when compiling for Win64.
		#define KM_PLATFORM_WIN64 1
	#else
		#define KM_PLATFORM_WIN32 1
	#endif
	#if defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
		#define KM_PROCESSOR_X86_64 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Windows on x64"
	#elif defined(_M_IX86) || defined(_X86_)
		#define KM_PROCESSOR_X86 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Windows on X86"
	#elif defined(_M_IA64) || defined(_IA64_)
		#define KM_PROCESSOR_IA64 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Windows on IA-64"
	#elif defined(_M_ARM)
		#define KM_ABI_ARM_WINCE 1
		#define KM_PROCESSOR_ARM32 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Windows on ARM"
	#elif defined(_M_ARM64)
		#define KM_PROCESSOR_ARM64 1
		#define KM_SYSTEM_LITTLE_ENDIAN 1
		#define KM_PLATFORM_DESCRIPTION "Windows on ARM64"
	#else //Possibly other Windows CE variants
		#error Unknown processor
		#error Unknown endianness
	#endif
	#if defined(__GNUC__)
		#define KM_ASM_STYLE_ATT 1
	#elif defined(_MSC_VER) || defined(__BORLANDC__) || defined(__ICL)
		#define KM_ASM_STYLE_INTEL 1
	#endif
	#define KM_PLATFORM_DESKTOP 1
	#define KM_PLATFORM_MICROSOFT 1

	#if defined(_KERNEL_MODE)
		#define KM_PLATFORM_WINDOWS_KERNEL 1
	#endif

	// WINAPI_FAMILY defines to support Windows 8 Metro Apps - mirroring winapifamily.h in the Windows 8 SDK
	#define KM_WINAPI_FAMILY_APP         1000
	#define KM_WINAPI_FAMILY_DESKTOP_APP 1001
	#define KM_WINAPI_FAMILY_GAMES       1006

	#if defined(WINAPI_FAMILY)
		#if defined(_MSC_VER)
			#pragma warning(push, 0)
		#endif
		#include <winapifamily.h>
		#if defined(_MSC_VER)
			#pragma warning(pop)
		#endif
		#if defined(WINAPI_FAMILY_DESKTOP_APP) && WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
			#define KM_WINAPI_FAMILY KM_WINAPI_FAMILY_DESKTOP_APP
		#elif defined(WINAPI_FAMILY_APP) && WINAPI_FAMILY == WINAPI_FAMILY_APP
			#define KM_WINAPI_FAMILY KM_WINAPI_FAMILY_APP
		#elif defined(WINAPI_FAMILY_GAMES) && WINAPI_FAMILY == WINAPI_FAMILY_GAMES
			#define KM_WINAPI_FAMILY KM_WINAPI_FAMILY_GAMES
		#else
			#error Unsupported WINAPI_FAMILY
		#endif
	#else
		#define KM_WINAPI_FAMILY KM_WINAPI_FAMILY_DESKTOP_APP
	#endif

	#define KM_WINAPI_PARTITION_DESKTOP   1
	#define KM_WINAPI_PARTITION_APP       1
	#define KM_WINAPI_PARTITION_GAMES    (KM_WINAPI_FAMILY == KM_WINAPI_FAMILY_GAMES)

	#define KM_WINAPI_FAMILY_PARTITION(Partition)   (Partition)

	// KM_PLATFORM_WINRT
	// This is a subset of Windows which is used for tablets and the "Metro" (restricted) Windows user interface.
	// WinRT doesn't doesn't have access to the Windows "desktop" API, but WinRT can nevertheless run on 
	// desktop computers in addition to tablets. The Windows Phone API is a subset of WinRT and is not included
	// in it due to it being only a part of the API.
	#if defined(__cplusplus_winrt)
		#define KM_PLATFORM_WINRT 1
	#endif

// Sun (Solaris)
// __SUNPRO_CC is defined by the Sun compiler.
// __sun is defined by the GCC compiler.
// __i386 is defined by the Sun and GCC compilers.
// __sparc is defined by the Sun and GCC compilers.
#else
	#error Unknown platform
	#error Unknown processor
	#error Unknown endianness
#endif

#ifndef KM_PROCESSOR_ARM
	#if defined(KM_PROCESSOR_ARM32) || defined(KM_PROCESSOR_ARM64) || defined(KM_PROCESSOR_ARM7)
		#define KM_PROCESSOR_ARM
	#endif
#endif

// KM_PLATFORM_PTR_SIZE
// Platform pointer size; same as sizeof(void*).
// This is not the same as sizeof(int), as int is usually 32 bits on
// even 64 bit platforms.
//
// _WIN64 is defined by Win64 compilers, such as VC++.
// _M_IA64 is defined by VC++ and Intel compilers for IA64 processors.
// __LP64__ is defined by HP compilers for the LP64 standard.
// _LP64 is defined by the GCC and Sun compilers for the LP64 standard.
// __ia64__ is defined by the GCC compiler for IA64 processors.
// __arch64__ is defined by the Sparc compiler for 64 bit processors.
// __mips64__ is defined by the GCC compiler for MIPS processors.
// __powerpc64__ is defined by the GCC compiler for PowerPC processors.
// __64BIT__ is defined by the AIX compiler for 64 bit processors.
// __sizeof_ptr is defined by the ARM compiler (armcc, armcpp).
//
#ifndef KM_PLATFORM_PTR_SIZE
	#if defined(__WORDSIZE) // Defined by some variations of GCC.
		#define KM_PLATFORM_PTR_SIZE ((__WORDSIZE) / 8)
	#elif defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(_M_IA64) || defined(__ia64__) || defined(__arch64__) || defined(__aarch64__) || defined(__mips64__) || defined(__64BIT__) || defined(__Ptr_Is_64)
		#define KM_PLATFORM_PTR_SIZE 8
	#elif defined(__CC_ARM) && (__sizeof_ptr == 8)
		#define KM_PLATFORM_PTR_SIZE 8
	#else
		#define KM_PLATFORM_PTR_SIZE 4
	#endif
#endif



// KM_PLATFORM_WORD_SIZE
// This defines the size of a machine word. This will be the same as
// the size of registers on the machine but not necessarily the same
// as the size of pointers on the machine. A number of 64 bit platforms
// have 64 bit registers but 32 bit pointers.
//
#ifndef KM_PLATFORM_WORD_SIZE
	#define KM_PLATFORM_WORD_SIZE KM_PLATFORM_PTR_SIZE
#endif

// KM_PLATFORM_MIN_MALLOC_ALIGNMENT
// This defines the minimal alignment that the platform's malloc 
// implementation will return. This should be used when writing custom
// allocators to ensure that the alignment matches that of malloc
#ifndef KM_PLATFORM_MIN_MALLOC_ALIGNMENT
	#if defined(KM_PLATFORM_APPLE)
		#define KM_PLATFORM_MIN_MALLOC_ALIGNMENT 16
	#elif defined(KM_PLATFORM_ANDROID) && defined(KM_PROCESSOR_ARM)
		#define KM_PLATFORM_MIN_MALLOC_ALIGNMENT 8
	#elif defined(KM_PLATFORM_ANDROID) && defined(KM_PROCESSOR_X86_64)
		#define KM_PLATFORM_MIN_MALLOC_ALIGNMENT 8
	#else
		#define KM_PLATFORM_MIN_MALLOC_ALIGNMENT (KM_PLATFORM_PTR_SIZE * 2)
	#endif
#endif


// KM_MISALIGNED_SUPPORT_LEVEL
// Specifies if the processor can read and write built-in types that aren't
// naturally aligned.
//    0 - not supported. Likely causes an exception.
//    1 - supported but slow.
//    2 - supported and fast.
//
#ifndef KM_MISALIGNED_SUPPORT_LEVEL
	#if defined(KM_PROCESSOR_X86_64)
		#define KM_MISALIGNED_SUPPORT_LEVEL 2
	#else
		#define KM_MISALIGNED_SUPPORT_LEVEL 0
	#endif
#endif

// Macro to determine if a Windows API partition is enabled. Always false on non Microsoft platforms.
#if !defined(KM_WINAPI_FAMILY_PARTITION)
	#define KM_WINAPI_FAMILY_PARTITION(Partition) (0)
#endif


// KM_CACHE_LINE_SIZE
// Specifies the cache line size broken down by compile target.
// This the expected best guess values for the targets that we can make at compilation time.

#ifndef KM_CACHE_LINE_SIZE
	#if   defined(KM_PROCESSOR_X86)
		#define KM_CACHE_LINE_SIZE 32    // This is the minimum possible value.
	#elif defined(KM_PROCESSOR_X86_64)
		#define KM_CACHE_LINE_SIZE 64    // This is the minimum possible value
	#elif defined(KM_PROCESSOR_ARM32)
		#define KM_CACHE_LINE_SIZE 32    // This varies between implementations and is usually 32 or 64.
	#elif defined(KM_PROCESSOR_ARM64)
		#define KM_CACHE_LINE_SIZE 64    // Cache line Cortex-A8  (64 bytes) http://shervinemami.info/armAssembly.html however this remains to be mostly an assumption at this stage
	#elif (KM_PLATFORM_WORD_SIZE == 4)
		#define KM_CACHE_LINE_SIZE 32    // This is the minimum possible value
	#else
		#define KM_CACHE_LINE_SIZE 64    // This is the minimum possible value
	#endif
#endif


#endif // INCLUDED_eaplatform_H









