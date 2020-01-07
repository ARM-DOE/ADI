SUBDIRS = src conf

EXTRA_DIST = Doxyfile.in

doxygen-docs: Doxyfile.in
	mkdir -p @prefix@/share/doc/@GROUP@/@PROJECT@
	doxygen Doxyfile

doxygen-clean:
	rm -rf @prefix@/share/doc/@GROUP@/@PROJECT@

MAINTAINERCLEANFILES = \
aclocal.m4 \
build-aux/ar-lib \
build-aux/config.guess \
build-aux/config.sub \
build-aux/depcomp \
build-aux/install-sh \
build-aux/ltmain.sh \
build-aux/missing \
config.h.in \
config.h.in~ \
configure \
m4/libtool.m4 \
m4/lt~obsolete.m4 \
m4/ltoptions.m4 \
m4/ltsugar.m4 \
m4/ltversion.m4 \
Makefile.in
