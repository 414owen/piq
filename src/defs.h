#pragma once

#ifdef TIME_ALL

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
