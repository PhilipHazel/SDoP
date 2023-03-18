dnl Process this file with autoconf to produce a configure script.

dnl This is required at the start; the name is the name of a file
dnl it should be seeing, to verify it is in the same directory.

AC_INIT
AC_CONFIG_SRCDIR([NOTICE])

dnl Checks for programs.

AC_PROG_CC
AC_PROG_INSTALL

dnl Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_TYPE_SIZE_T

dnl ---- JPEG support ----

SUPPORT_JPEG=0
support_jpeg=no

dnl Handle --disable-jpeg
AC_ARG_ENABLE(jpeg,
              AS_HELP_STRING([--disable-jpeg],
                             [disable jpeg support]),
              , enable_jpeg=yes)

if test "x$enable_jpeg" = "xyes"
then

dnl Check for the availability of jpeglib
AC_CHECK_HEADERS([jpeglib.h], [HAVE_JPEGLIB_H=1])
AC_CHECK_HEADERS([jerror.h], [HAVE_JERROR_H=1])
AC_CHECK_LIB([jpeg], [jpeg_std_error], [HAVE_LIBJPEG=1])

if test "${HAVE_JPEGLIB_H}${HAVE_JERROR_H}${HAVE_LIBJPEG}" = "111"; then
  SUPPORT_JPEG=1
  support_jpeg=yes
  if test "${LIBS}" = ""; then LIBS=-ljpeg; else LIBS="${LIBS} -ljpeg"; fi
fi
fi

dnl ---- PNG support ----

SUPPORT_PNG=0
support_png=no

dnl Handle --disable-png
AC_ARG_ENABLE(png,
              AS_HELP_STRING([--disable-png],
                             [disable png support]),
              , enable_png=yes)

if test "x$enable_png" = "xyes"
then

dnl Check for the availability of pnglib
AC_CHECK_HEADERS([png.h], [HAVE_PNG_H=1])
AC_CHECK_LIB([png], [png_sig_cmp], [HAVE_LIBPNG=1])

if test "${HAVE_PNG_H}${HAVE_LIBPNG}" = "11"; then
  SUPPORT_PNG=1
  support_png=yes
  if test "${LIBS}" = ""; then LIBS=-lpng; else LIBS="${LIBS} -lpng"; fi
fi
fi

dnl ---- Final stuff ----

AC_SUBST(SUPPORT_JPEG)
AC_SUBST(SUPPORT_PNG)
AC_CONFIG_FILES(Makefile)
AC_OUTPUT

dnl Afterwards, print a summary message

cat <<EOF

Configuration summary:

    Install prefix ......... : ${prefix}
    C preprocessor ......... : ${CPP}
    C compiler ............. : ${CC}
    Linker ................. : ${LD}
    C preprocessor flags ... : ${CPPFLAGS}
    C compiler flags ....... : ${CFLAGS}
    Linker flags ........... : ${LDFLAGS}
    Extra libraries ........ : ${LIBS}
    Support JPEG ........... : ${support_jpeg}
    Support PNG ............ : ${support_png}

EOF

dnl end configure.ac
