SUBDIRS = . test

# Now a shared library AND a GModule
lib_LTLIBRARIES = libgnc-backend-sql.la

# "${top_srcdir}/src/debug" below is for the splint header
AM_CFLAGS = \
  -I.. -I../.. \
  -DLOCALE_DIR=\""$(datadir)/locale"\" \
  -I${top_srcdir}/src/backend \
  -I${top_srcdir}/src/debug \
  -I${top_srcdir}/src/engine \
  -I${top_srcdir}/src/core-utils\
  -I${top_srcdir}/lib/libc\
  -I${top_srcdir}/src/libqof/qof \
  ${GLIB_CFLAGS} \
  ${GUILE_INCS} \
  ${GCONF_CFLAGS} \
  ${WARN_CFLAGS}

libgnc_backend_sql_la_SOURCES = \
  gnc-backend-sql.c \
  gnc-account-sql.c \
  gnc-book-sql.c \
  gnc-budget-sql.c \
  gnc-commodity-sql.c \
  gnc-lots-sql.c \
  gnc-price-sql.c \
  gnc-recurrence-sql.c \
  gnc-schedxaction-sql.c \
  gnc-slots-sql.c \
  gnc-transaction-sql.c \
  escape.c

noinst_HEADERS = \
  gnc-account-sql.h \
  gnc-backend-sql.h \
  gnc-book-sql.h \
  gnc-budget-sql.h \
  gnc-commodity-sql.h \
  gnc-lots-sql.h \
  gnc-price-sql.h \
  gnc-recurrence-sql.h \
  gnc-schedxaction-sql.h \
  gnc-slots-sql.h \
  gnc-transaction-sql.h \
  escape.h

libgnc_backend_sql_la_LIBADD = \
   ${GLIB_LIBS} \
   ${GUILE_LIBS} \
   ${GCONF_LIBS} \
   ${top_builddir}/src/engine/libgncmod-engine.la \
   ${top_builddir}/src/libqof/qof/libgnc-qof.la

INCLUDES = -DG_LOG_DOMAIN=\"gnc.backend.sql\"
