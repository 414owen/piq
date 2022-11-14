#include <predef/predef.h>

const char *const program_name = "lang";

#if PREDEF_OS_WINDOWS
const char path_sep = '\\';
#else
const char path_sep = '/';
#endif