#pragma once

#include "version/appversion.h"

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <stddef.h>
	#define NOINLINE __declspec(noinline)

	#define snprintf _snprintf
	#define strcasecmp _stricmp
	#define rotl _rotl
	#define rotr _rotr
	#define rotl64 _rotl64
	#define rotr64 _rotr64

#else //WIN32
	#include <arpa/inet.h>
	#include <dlfcn.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <netinet/in.h>
	#include <sys/sysinfo.h>
	#include <sys/time.h>
	#define NOINLINE __attribute__((noinline))

	inline uint32_t rotl(uint32_t x, int r)
	{
		return (x << r) | (x >> (32 - r));
	}

	inline uint32_t rotr(uint32_t x, int r)
	{
		return (x >> r) | (x << (32 - r));
	}

	inline uint64_t rotl64(uint64_t x, int r)
	{
		return (x << r) | (x >> (64 - r));
	}

	inline uint64_t rotr64(uint64_t x, int r)
	{
		return (x >> r) | (x << (64 - r));
	}

#endif //WIN32
