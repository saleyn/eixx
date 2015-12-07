SUBDIRS = src test
ACLOCAL_AMFLAGS = -I build-aux

pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = src/eixx.pc

clean-local:
	-rm -fr @default_prefix@

docs: build-aux/Doxyfile
	$(MKDIR_P) @docdir@
	doxygen build-aux/Doxyfile

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
    src/eixx.pc

EXTRA_DIST = README LICENSE LICENSE.header license.sh \
			 build-aux/boost.m4 build-aux/Doxyfile.in \
             bootstrap

installdir = $(prefix)
dist_doc_DATA = README.md LICENSE

.PHONY: docs doc
