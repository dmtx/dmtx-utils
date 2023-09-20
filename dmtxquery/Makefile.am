AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = -Wshadow -Wall -pedantic

bin_PROGRAMS = dmtxquery
noinst_PROGRAMS = dmtxquery.debug

dmtxquery_SOURCES = dmtxquery.c dmtxquery.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxquery_CFLAGS = $(DMTX_CFLAGS)
dmtxquery_LDFLAGS = $(DMTX_LIBS)
dmtxquery_LDADD = $(LIBOBJS)

dmtxquery_debug_SOURCES = dmtxquery.c dmtxquery.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxquery_debug_CFLAGS = $(DMTX_CFLAGS)
dmtxquery_debug_LDFLAGS = -static $(DMTX_LIBS)
dmtxquery_debug_LDADD = $(LIBOBJS)
