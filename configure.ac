#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([librpt], [1.0.0], [rajenpandit@gmail.com])
m4_define([librpt_lt_current],	[1])
m4_define([librpt_lt_revision],	[0])
m4_define([librpt_lt_age],    	[0])
LT_CURRENT=librpt_lt_current
AC_SUBST(LT_CURRENT)
LT_REVISION=librpt_lt_revision
AC_SUBST(LT_REVISION)
LT_AGE=librpt_lt_age
AC_SUBST(LT_AGE)
LT_INIT([])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src/thread_pool/task.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_14

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h netdb.h stdlib.h sys/ioctl.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset socket])
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/tcp_server/Makefile
                 src/thread_pool/Makefile])
AC_PROG_RANLIB
AC_OUTPUT
