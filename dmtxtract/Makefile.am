AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = -Wshadow -Wall -pedantic

bin_PROGRAMS = dmtxtract
noinst_PROGRAMS = dmtxtract.debug

dmtxtract_SOURCES = dmtxtract.c dmtxtract.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxtract_CFLAGS = $(DMTX_CFLAGS) $(MAGICK_CFLAGS) -D_MAGICK_CONFIG_H
dmtxtract_LDFLAGS = $(DMTX_LIBS) $(MAGICK_LIBS)
dmtxtract_LDADD = $(LIBOBJS)

dmtxtract_debug_SOURCES = dmtxtract.c dmtxtract.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxtract_debug_CFLAGS = $(DMTX_CFLAGS) $(MAGICK_CFLAGS) -D_MAGICK_CONFIG_H
dmtxtract_debug_LDFLAGS = -static $(DMTX_LIBS) $(MAGICK_LIBS)
dmtxtract_debug_LDADD = $(LIBOBJS)
