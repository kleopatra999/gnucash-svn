#!/bin/sh
# Run this to generate all the initial makefiles, etc.

DIE=0

AUTOCONF=${AUTOCONF:-autoconf}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
#GETTEXTIZE=${GETTEXTIZE:-gettextize}
#INTLTOOLIZE=${INTLTOOLIZE:-intltoolize}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
LIBTOOL=${LIBTOOL:-libtool}

if [ -n "$GNOME2_PATH" ]; then
	ACLOCAL_FLAGS="-I $GNOME2_PATH/share/aclocal $ACLOCAL_FLAGS"
	PATH="$GNOME2_PATH/bin:$PATH"
	export PATH
fi

(${AUTOCONF} --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed to compile GnuCash."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

#GETTEXTIZE_VERSION=`${GETTEXTIZE} --version`
#gettextize_major_version=`echo ${GETTEXTIZE_VERSION} | \
#	sed 's/^.*GNU gettext.* \([0-9]*\)\.\([0-9]*\).\([0-9]*\).*$/\1/'`
#gettextize_minor_version=`echo ${GETTEXTIZE_VERSION} | \
#	sed 's/^.*GNU gettext.* \([0-9]*\)\.\([0-9]*\).\([0-9]*\).*$/\2/'`
#if [  $gettextize_major_version -gt 0   -o \
#      $gettextize_minor_version -gt 10  ]; then
#  INTL="--intl";
#else
#  INTL="";
#fi

#(grep "^AM_PROG_LIBTOOL" $srcdir/configure.in >/dev/null) && {
#  (${LIBTOOL} --version) < /dev/null > /dev/null 2>&1 || {
#    echo
#    echo "**Error**: You must have \`libtool' installed to compile GnuCash."
#    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.4.2.tar.gz"
#    echo "(or a newer version if it is available)"
#    DIE=1
#  }
#}

#grep "^AM_GNU_GETTEXT" $srcdir/configure.in >/dev/null && {
#  grep "sed.*POTFILES" $srcdir/configure.in >/dev/null || \
#  (${GETTEXT} --version) < /dev/null > /dev/null 2>&1 || {
#    echo
#    echo "**Error**: You must have \`gettext' installed to compile GnuCash."
#    echo "Get ftp://alpha.gnu.org/gnu/gettext-0.10.35.tar.gz"
#    echo "(or a newer version if it is available)"
#    DIE=1
#  }
#}

#grep "^AM_GNOME_GETTEXT" $srcdir/configure.in >/dev/null && {
#  grep "sed.*POTFILES" $srcdir/configure.in >/dev/null || \
#  (${GETTEXT} --version) < /dev/null > /dev/null 2>&1 || {
#    echo
#    echo "**Error**: You must have \`gettext' installed to compile GnuCash."
#    echo "Get ftp://alpha.gnu.org/gnu/gettext-0.10.35.tar.gz"
#    echo "(or a newer version if it is available)"
#    DIE=1
#  }
#}

(${AUTOMAKE} --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed to compile GnuCash."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.4.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (${ACLOCAL} --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.4.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0"' command line."
  echo
fi

case $CC in
xlc )
  am_opt=--include-deps;;
esac

for coin in `find $srcdir -name configure.in -print`
do 
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
    echo skipping $dr -- flagged as no auto-gen
  else
    echo processing $dr
    macrodirs=`sed -n -e 's,AM_ACLOCAL_INCLUDE(\(.*\)),\1,gp' < $coin`
    ( cd $dr
      macrosdir=`find . -name macros -print`
      for i in $macrodirs; do
	if test -f $i/gnome-gettext.m4; then
	  DELETEFILES="$DELETEFILES $i/gnome-gettext.m4"
	fi
      done

      echo "deletefiles is $DELETEFILES"
      aclocalinclude="$ACLOCAL_FLAGS"
      for k in $aclocalinclude; do
  	if test -d $k; then
	  if [ -f $k/gnome.m4 -a "$GNOME_INTERFACE_VERSION" = "1" ]; then
	    rm -f $DELETEFILES
	  fi
        fi
      done
      for k in $macrodirs; do
  	if test -d $k; then
          aclocalinclude="$aclocalinclude -I $k"
	  if [ -f $k/gnome.m4 -a "$GNOME_INTERFACE_VERSION" = "1" ]; then
	    rm -f $DELETEFILES
	  fi
        fi
      done
      if grep "^AM_GNU_GETTEXT" configure.in >/dev/null; then
        if grep "sed.*POTFILES" configure.in >/dev/null; then
          : do nothing -- we still have an old unmodified configure.in
        else
          echo "Creating $dr/aclocal.m4 ..."
          test -r $dr/aclocal.m4 || touch $dr/aclocal.m4
          echo "Running gettextize...  Ignore non-fatal messages."
          echo "no" | gettextize --force --copy
          echo "Making $dr/aclocal.m4 writable ..."
          test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
        fi
      fi
      if grep "^AM_GNOME_GETTEXT" configure.in >/dev/null; then
        echo "Creating $dr/aclocal.m4 ..."
        test -r $dr/aclocal.m4 || touch $dr/aclocal.m4
        echo "Running gettextize...  Ignore non-fatal messages."
        echo "no" | gettextize --force --copy
        echo "Making $dr/aclocal.m4 writable ..."
        test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
      fi
      if grep "^AM_GLIB_GNU_GETTEXT" configure.in >/dev/null; then
        echo "Creating $dr/aclocal.m4 ..."
        test -r $dr/aclocal.m4 || touch $dr/aclocal.m4
        echo "Running gettextize...  Ignore non-fatal messages."
        echo "no" | glib-gettextize --force --copy
        echo "Making $dr/aclocal.m4 writable ..."
        test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
      fi
      if grep "^AC_PROG_INTLTOOL" configure.in >/dev/null; then
        echo "Running intltoolize ..."
        intltoolize --copy
      fi
      if grep "^A[CM]_PROG_LIBTOOL" configure.in >/dev/null; then
        echo "Running libtoolize..."
        libtoolize --force --copy
      fi
      echo "Running $ACLOCAL $aclocalinclude ..."
      $ACLOCAL $aclocalinclude
      if grep "^AM_CONFIG_HEADER" configure.in >/dev/null; then
        echo "Running autoheader..."
        autoheader
      fi
      echo "Running $AUTOMAKE --gnu $am_opt ..."
      $AUTOMAKE --add-missing --gnu $am_opt
      echo "Running autoconf ..."
      autoconf
    )
  fi
done

conf_flags="--enable-maintainer-mode --enable-error-on-warning --enable-compile-warnings" # --enable-iso-c

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME || exit 1
else
  echo Skipping configure process.
fi
