AC_INIT([libdmtx-utils], [0.7.3], [mike@dragonflylogic.com])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
   Makefile
])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_SEARCH_LIBS([atan2], [m] ,[], AC_MSG_ERROR([libdmtx-utils requires libm]))

AC_ARG_ENABLE(
   [dmtxquery],
   AS_HELP_STRING([--disable-dmtxquery], [do not build the dmtxquery command line utility]),
   [dmtxquery="$enableval"],
   [dmtxquery="yes"]
)
AM_CONDITIONAL([ENABLE_DMTXQUERY], [test x$dmtxquery = xyes])

AC_ARG_ENABLE(
   [dmtxread],
   AS_HELP_STRING([--disable-dmtxread], [do not build the dmtxread command line utility]),
   [dmtxread="$enableval"],
   [dmtxread="yes"]
)
AM_CONDITIONAL([ENABLE_DMTXREAD], [test x$dmtxread = xyes])

AC_ARG_ENABLE(
   [dmtxwrite],
   AS_HELP_STRING([--disable-dmtxwrite], [do not build the dmtxwrite command line utility]),
   [dmtxwrite="$enableval"],
   [dmtxwrite="yes"]
)
AM_CONDITIONAL([ENABLE_DMTXWRITE], [test x$dmtxwrite = xyes])

if test x$dmtxread = xyes \
      -o x$dmtxwrite = xyes \
      -o x$dmtxquery = xyes; then
   AC_CHECK_HEADERS([sysexits.h])
   AC_CHECK_HEADERS([getopt.h])
   AC_CHECK_FUNCS([getopt_long])
fi

if test x$dmtxread = xyes \
      -o x$dmtxwrite = xyes; then
   ifdef([PKG_CHECK_MODULES],
         [PKG_CHECK_MODULES(
            MAGICK,
            Wand >= 6.2.4,
            true,
            AC_MSG_ERROR([[dmtxread/dmtxwrite requires Wand >= 6.2.4]]))],
         [AC_MSG_ERROR([Building dmtxread/dmtxwrite requires a working autoconf/pkg-config])]
   )
   AC_SUBST(MAGICK_CFLAGS)
   AC_SUBST(MAGICK_LIBS)
fi

if test x$dmtxquery = xyes; then
   AC_CONFIG_FILES([dmtxquery/Makefile])
fi

if test x$dmtxread = xyes; then
   AC_CONFIG_FILES([dmtxread/Makefile])
fi

if test x$dmtxwrite = xyes; then
   AC_CONFIG_FILES([dmtxwrite/Makefile])
fi

AC_OUTPUT
