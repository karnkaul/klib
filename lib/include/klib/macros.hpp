#pragma once

#if defined(_MSC_VER)
#define KLIB_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define KLIB_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#define KLIB_PRAGMA_STRINGIFY(x) #x
#if defined(__GNUG__) && !defined(__clang__)
#define KLIB_GCC_IGNORE_PUSH(warning) _Pragma("GCC diagnostic push") _Pragma(KLIB_PRAGMA_STRINGIFY(GCC diagnostic ignored warning))
#define KLIB_GCC_IGNORE_POP() _Pragma("GCC diagnostic pop")
#else
#define KLIB_GCC_IGNORE_PUSH(warning)
#define KLIB_GCC_IGNORE_POP()
#endif
