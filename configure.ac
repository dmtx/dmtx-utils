AC_INIT([dmtx-utils], [0.7.7], [https://github.com/dmtx/dmtx-utils/issues], [dmtx-utils], [https://github.com/dmtx/dmtx-utils])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_LIBOBJ_DIR([common])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_PATH_PROG([PKG_CONFIG], [pkg-config], [no])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

ifdef([PKG_CHECK_MODULES], [], [AC_MSG_ERROR([Building dmtxread/dmtxwrite requires a working autoconf/pkg-config])])

AC_SEARCH_LIBS([atan2], [m] ,[], AC_MSG_ERROR([dmtx-utils requires libm]))

PKG_CHECK_MODULES(DMTX, libdmtx >= 0.7.0, [], AC_MSG_ERROR([dmtxread/dmtxwrite requires libdmtx >= 0.7.0]))
AC_SUBST(DMTX_CFLAGS)
AC_SUBST(DMTX_LIBS)

AC_CHECK_HEADERS([sysexits.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_FUNC([getopt_long], [], [ AC_LIBOBJ([getopt]) AC_LIBOBJ([getopt1]) ])

AC_ARG_ENABLE(
   [dmtxquery],
   AS_HELP_STRING([--disable-dmtxquery], [do not build the dmtxquery utility]),
   [dmtxquery="$enableval"],
   [dmtxquery="yes"]
)

AC_ARG_ENABLE(
   [dmtxread],
   AS_HELP_STRING([--disable-dmtxread], [do not build the dmtxread utility]),
   [dmtxread="$enableval"],
   [dmtxread="yes"]
)

AC_ARG_ENABLE(
   [dmtxwrite],
   AS_HELP_STRING([--disable-dmtxwrite], [do not build the dmtxwrite utility]),
   [dmtxwrite="$enableval"],
   [dmtxwrite="yes"]
)

AM_CONDITIONAL([ENABLE_DMTXQUERY], [test x$dmtxquery = xyes])
AM_CONDITIONAL([ENABLE_DMTXREAD], [test x$dmtxread = xyes])
AM_CONDITIONAL([ENABLE_DMTXWRITE], [test x$dmtxwrite = xyes])

if test x$dmtxquery = xyes; then
   AC_CONFIG_FILES([dmtxquery/Makefile])
fi

if test x$dmtxread = xyes; then
   AC_CONFIG_FILES([dmtxread/Makefile])
fi

if test x$dmtxwrite = xyes; then
   AC_CONFIG_FILES([dmtxwrite/Makefile])
fi

if test x$dmtxread = xyes -o x$dmtxwrite = xyes; then
   PKG_CHECK_MODULES(MAGICK, MagickWand >= 6.2.4, [], AC_MSG_ERROR([dmtxread/dmtxwrite requires MagickWand >= 6.2.4]))
   AH_TEMPLATE([IM_API_7], [Define to 1 if version of installed ImageMagick library is 7.x])
   AS_CASE(
     [`${PKG_CONFIG} --modversion MagickWand`],
     [7.[[0-9]]*],
     [AC_DEFINE([IM_API_7], [1])]
   )

   AC_SUBST(MAGICK_CFLAGS)
   AC_SUBST(MAGICK_LIBS)
fi

AC_OUTPUT
