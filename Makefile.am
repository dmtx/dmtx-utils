# Packaging commands (all run from libdmtx-utils root):
# $ make distclean
# $ make dist-bzip2
# $ make dist-gzip
# $ script/dist-image.sh

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wshadow -Wall -pedantic -ansi

SUBDIRS = . util

dist_man_MANS = man/dmtxread.1 man/dmtxwrite.1 man/dmtxquery.1

EXTRA_DIST = KNOWNBUG \
	README.cygwin \
	README.freebsd \
	README.linux \
	README.mingw \
	README.osx \
	README.unix \
	README.visualc \
	m4 \
	project \
	script/check_all.sh \
	script/check_comments.sh \
	script/check_copyright.sh \
	script/check_headers.pl \
	script/check_keyword.sh \
	script/check_license.sh \
	script/check_spacing.sh \
	script/check_todo.sh \
	script/check_whitespace.sh \
	script/dist-image.sh
