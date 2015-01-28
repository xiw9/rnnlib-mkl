#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([RECURRENT NEURAL NETWORK LIBRARY], [1.0], [graves@in.tum.de])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
if test -z $CXXFLAGS; then
    CXXFLAGS='-O3 -march=native'
fi
AC_PROG_CXX

# Checks for libraries.
AC_CHECK_LIB([stdc++], [main])
AC_CHECK_LIB([m], [exp])
AC_CHECK_LIB(netcdf, main)
AC_CHECK_LIB(netcdf_c++, main)
AC_CHECK_LIB(pthread, main)
AC_CHECK_LIB(mkl_rt, main, LIBS="-lmkl_intel_lp64 -lmkl_core -lmkl_sequential $LIBS")

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([time.h malloc.h mkl.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([floor pow sqrt])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
