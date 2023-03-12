#include <predef/predef.h>

const char *const program_name = "lang";

#if PREDEF_OS_WINDOWS
const char path_sep = '\\';
#else
const char path_sep = '/';
#endif

const char *issue_tracker_url = "https://github.com/414owen/lang-c/issues";
