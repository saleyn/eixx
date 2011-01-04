SUBDIRS = src test
ACLOCAL_AMFLAGS = -I build-aux

pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = src/eixx-${PACKAGE_VERSION}.pc

clean-local:
	-rm -fr @default_prefix@

docs: build-aux/Doxyfile
	$(MKDIR_P) @docdir@
	doxygen build-aux/Doxyfile
	(cd lib/dma/src && $(MAKE) $@)

build-aux/Doxyfile: build-aux/Doxyfile.in
	$(SED) -e 's|[@]docdir@|${docdir}|g' \
		   -e 's|[@]PACKAGE@|${PACKAGE}|g' \
		   -e 's|[@]PACKAGE_VERSION@|${PACKAGE_VERSION}|g' $< > "$@"

CLEANFILES = stamp-h1
DISTCLEANFILES = config.h.in~ config.log build-aux/Doxyfile
MAINTAINERCLEANFILES = configure build-aux/l*.m4 build-aux/*.sh \
	build-aux/config.* build-aux/missing build-aux/depcomp \
	build-aux/install-sh build-aux/Doxyfile \
	aclocal.m4 Makefile.in src/Makefile.in test/Makefile.in \
    src/eixx-${PACKAGE_VERSION}.pc

EXTRA_DIST = README build-aux/boost.m4 build-aux/Doxyfile.in \
             bootstrap

.PHONY: docs doc
