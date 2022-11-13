#include <direct.h>

// http://msdn.microsoft.com/en-us/library/2fkk4dzw.aspx
#define mkdir(pathname, mode) _mkdir(pathname);
