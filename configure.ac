#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.63)
AC_INIT(yacmd, 1.0, yaochuanqi888@163.com)
AM_INIT_AUTOMAKE(yacmd, 1.0)
AC_CONFIG_SRCDIR([src/keyword.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strstr])
PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.4)

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT
