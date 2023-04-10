// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdint.h>

#define HASH_BITS 32
#define HASH_T uint32_t
typedef HASH_T hash_t;

hash_t hash_newtype(const void *key_p, const void *ctx_p);
hash_t hash_stored_type(const void *key_p, const void *ctx_p);
hash_t hash_binding(const void *binding_p, const void *ctx_p);
hash_t hash_stored_binding(const void *binding_ind_p, const void *ctx_p);
