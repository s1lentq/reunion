//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the
// public domain. The author hereby disclaims copyright to this source
// code.

#pragma once

#include "reunion_shared.h"

//-----------------------------------------------------------------------------

void MurmurHash3_x86_32 (const void *key, int len, uint32 seed, void *out);

void MurmurHash3_x86_128(const void *key, const int len, uint32 seed, void *out);

void MurmurHash3_x64_128(const void *key, const int len, const uint32 seed, void *out);

//-----------------------------------------------------------------------------
