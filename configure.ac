#  -*- Autoconf -*-

AC_PREREQ([2.63])
AC_INIT([eixx], [0.1], [BUG-REPORT-ADDRESS])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Wno-portability])
AM_CONFIG_HEADER(config.h)
AX_PREFIX_CONFIG_H(include/eixx/config.h)

AC_SUBST([eixxdir], [$libdir])

# Default prefix
AC_PREFIX_DEFAULT(`pwd`)
AC_SUBST([default_prefix], [${PWD}/install])
test "$prefix" = "NONE" && prefix="$default_prefix"
test "$sysconfdir" = "\${prefix}/etc" && sysconfdir='../etc'
test "$scriptdir" = "" && scriptdir='../lib'

# Options

AC_ARG_VAR([ERLC_FLAGS], [general flags to prepend to ERLC_FLAGS])

ERLC_FLAGS="${ERLC_FLAGS} +debug_info"
# CXXFLAGS="${CXXFLAGS% } -MMD -Wall -fno-strict-aliasing -fpermissive -Wl,-V"
CXXFLAGS="${CXXFLAGS% } -MMD -Wall -fno-strict-aliasing"

AC_ARG_ENABLE(debug,
     AC_HELP_STRING([--enable-debug],[enable debug [[default=yes]]]),
     [ if test "x$enable_debug" = "xyes" -o -z "x$enable_debug"; then
           # CXXFLAGS="${CXXFLAGS% } -ggdb -O0 -Wall -fno-default-inline -fno-inline -fno-inline-functions"
           CXXFLAGS="${CXXFLAGS% } -g -O0"
       elif test "x$enable_debug" = "xno"; then
           ERLC_FLAGS="${ERLC_FLAGS//+debug_info}"
       fi
     ],
)

AC_ARG_ENABLE(optimize,
     AC_HELP_STRING([--enable-optimize],[enable code optimization [[default=yes]]]),
     [ if test "x$enable_optimize" = "xyes" ; then
           CXXFLAGS="${CXXFLAGS% } -O3"
       fi
     ],
)

AC_ARG_ENABLE(warnings,
     AC_HELP_STRING([--enable-warnings],[enable Wall compiling [[default=yes]]]),
     [ if test "x$enable_warnings" = "xyes" -o -z "$enable_warnings"; then
           CXXFLAGS="${CXXFLAGS% } -Wall -Werror"
       fi
     ],
)

# libtool
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CXX
AC_PROG_SED

#AC_CONFIG_HEADERS([config.h])

# Checks for libraries.

AX_BOOST_BASE([1.49.0], [], [AC_MSG_ERROR("BOOST library version >= 1.49.0 not found!")])
AX_BOOST_SYSTEM
AX_BOOST_THREAD
AX_BOOST_ASIO
AX_BOOST_DATE_TIME
AX_BOOST_UNIT_TEST_FRAMEWORK

# eixx requires md5 support
AX_LIB_CRYPTO([yes])
AC_CHECK_HEADERS([openssl/md5.h])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday socket sqrt])

dnl ------------------------------------------------------------------
dnl Erlang environment.
dnl ------------------------------------------------------------------

AC_MSG_NOTICE([Erlang environment])

dnl Check for erl_interface (used by port drivers).

AC_ERLANG_CHECK_LIB([kernel],, 
                    [AC_MSG_ERROR([kernel was not found!])])
AC_ERLANG_CHECK_LIB([stdlib],, 
                    [AC_MSG_ERROR([stdlib was not found!])])
AC_ERLANG_CHECK_LIB([sasl],, 
                    [AC_MSG_ERROR([sasl was not found!])])
AC_ERLANG_CHECK_LIB([erl_interface],, 
                    [AC_MSG_ERROR([erl_interface was not found!])])

AC_SUBST(ERL_CALL,[$ERLANG_LIB_DIR_erl_interface]/bin/erl_call)

dnl Available flags.
AC_ARG_WITH([erlang],
	AC_HELP_STRING([--with-erlang=PATH],
	    [PATH to Erlang installation (optional)]),
	with_erlang=${withval%/},
	with_erlang="")

dnl erl(1) is used to compile Erlang modules.
if test "x${with_erlang}" = "x"; then
	AC_ERLANG_PATH_ERL
	AC_ERLANG_PATH_ERLC
    AC_ERLANG_SUBST_ROOT_DIR
else
	erl_path="${with_erlang}/bin"
	AC_ERLANG_PATH_ERL(,  [$erl_path$PATH_SEPARATOR$PATH])
	AC_ERLANG_PATH_ERLC(, [$erl_path$PATH_SEPARATOR$PATH])
    AC_ERLANG_SUBST_ROOT_DIR
fi

if test "x${ERL}" = "x"; then
	AC_MSG_ERROR([
Erlang not found. Fill the ERL variable with erl(1) path or provide
Erlang prefix with --with-erlang.])
fi

AC_SUBST([EI_LDFLAGS], ["-L${ERLANG_LIB_DIR_erl_interface}/lib"])
AC_SUBST([EI_LIB],     ["-lei"])

AC_CHECK_HEADERS([${ERLANG_LIB_DIR_erl_interface}/src/epmd/ei_epmd.h],
   [ AC_DEFINE([HAVE_EI_EPMD], [1],
       [erl_interface/src/epmd/ei_epmd.h is available])
     AC_SUBST([EI_CPPFLAGS],
         ["-I${ERLANG_LIB_DIR_erl_interface}/include -I${ERLANG_LIB_DIR_erl_interface}/src"])
   ],
   [
     AC_SUBST([EI_CPPFLAGS],
         ["-I ${ERLANG_LIB_DIR_erl_interface}/include"])
   ])

dnl Get ERTS version.
ERLANG_CHECK_ERTS
ERLANG_CHECK_RELEASE
AC_ERLANG_SUBST_ERTS_VER

dnl ------------------------------------------------------------------
dnl Output.
dnl ------------------------------------------------------------------

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/eixx.pc:src/eixx.pc.in])
AC_CONFIG_FILES([test/Makefile])

AC_OUTPUT

echo
echo "========================================================================"
echo "   CXXFLAGS: ${CXXFLAGS}" $BOOST_CPPFLAGS
echo "      BOOST: $BOOST_ROOT"
echo "     Erlang: $ERLANG_ROOT_DIR"
echo "------------------------------------------------------------------------"
echo "             Configuration completed successfully!                      "
echo "========================================================================"
echo
