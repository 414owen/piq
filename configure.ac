#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([lang-c], [0.0.1], [https://github.com/414owen/lang-c/])
AC_CONFIG_SRCDIR([typecheck.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace 'main' with a function in '-lreadline':
AC_CHECK_LIB([readline], [main])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([inttypes.h stdint.h unistd.h])

AC_CONFIG_AUX_DIR([build-aux])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_MODE_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime memset mkdir strchr strdup])

AM_INIT_AUTOMAKE([-Wall -Werror -Wextra -pipe])

AC_CONFIG_FILES([Makefile
                 hedley/test/Makefile])
AC_OUTPUT
