#pragma once

#if defined(_MSC_VER)
#define KLIB_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define KLIB_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
