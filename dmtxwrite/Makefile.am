AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = -Wshadow -Wall -pedantic

bin_PROGRAMS = dmtxwrite
noinst_PROGRAMS = dmtxwrite.debug

dmtxwrite_SOURCES = dmtxwrite.c dmtxwrite.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxwrite_CFLAGS = $(DMTX_CFLAGS) $(MAGICK_CFLAGS) -D_MAGICK_CONFIG_H
dmtxwrite_LDFLAGS = $(DMTX_LIBS) $(MAGICK_LIBS)
dmtxwrite_LDADD = $(LIBOBJS)

dmtxwrite_debug_SOURCES = dmtxwrite.c dmtxwrite.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxwrite_debug_CFLAGS = $(DMTX_CFLAGS) $(MAGICK_CFLAGS) -D_MAGICK_CONFIG_H
dmtxwrite_debug_LDFLAGS = -static $(DMTX_LIBS) $(MAGICK_LIBS)
dmtxwrite_debug_LDADD = $(LIBOBJS)
