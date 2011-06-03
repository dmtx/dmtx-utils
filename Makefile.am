# Packaging commands (all run from dmtx-utils root):
# $ make distclean
# $ make dist-bzip2
# $ make dist-gzip

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wshadow -Wall -pedantic -ansi

if ENABLE_DMTXQUERY
   DMTXQUERY_DIR = dmtxquery
endif

if ENABLE_DMTXREAD
   DMTXREAD_DIR = dmtxread
endif

if ENABLE_DMTXWRITE
   DMTXWRITE_DIR = dmtxwrite
endif

SUBDIRS = . $(DMTXQUERY_DIR) $(DMTXREAD_DIR) $(DMTXWRITE_DIR)

dist_man_MANS = man/dmtxread.1 man/dmtxwrite.1 man/dmtxquery.1

EXTRA_DIST = KNOWNBUG \
	README.cygwin \
	README.freebsd \
	README.linux \
	README.mingw \
	README.osx \
	README.unix \
	m4 \
	script/check_all.sh \
	script/check_comments.sh \
	script/check_copyright.sh \
	script/check_headers.pl \
	script/check_license.sh \
	script/check_spacing.sh \
	script/check_todo.sh \
	script/check_whitespace.sh
