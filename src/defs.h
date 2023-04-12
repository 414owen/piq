// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#ifdef TIME_ALL

#if (__STDC_VERSION__ >= 201112L)
#define STD_C11
#endif

#ifndef TIME_TOKENIZER
#define TIME_TOKENIZER
#endif

#ifndef TIME_PARSER
#define TIME_PARSER
#endif

#ifndef TIME_NAME_RESOLUTION
#define TIME_NAME_RESOLUTION
#endif

#ifndef TIME_TYPECHECK
#define TIME_TYPECHECK
#endif

#ifndef TIME_CODEGEN
#define TIME_CODEGEN
#endif

#ifndef TIME_ANY
#define TIME_ANY
#endif

#endif

#ifdef TIME_TOKENIZER
#ifndef TIME_ANY
#define TIME_ANY
#endif
#endif

#ifdef TIME_PARSER
#ifndef TIME_ANY
#define TIME_ANY
#endif
#endif

#ifdef TIME_NAME_RESOLUTION
#ifndef TIME_ANY
#define TIME_ANY
#endif
#endif

#ifdef TIME_TYPECHECK
#ifndef TIME_ANY
#define TIME_ANY
#endif
#endif

#ifdef TIME_CODEGEN
#ifndef TIME_ANY
#define TIME_ANY
#endif
#endif
